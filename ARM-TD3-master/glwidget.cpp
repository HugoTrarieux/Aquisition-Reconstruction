#include <QMessageBox>
#include <QString>
#include <QTransform>
#include <QtGui>

#include "glwidget.h"

#include <iostream>

#include <fstream>
using namespace std;

GLWidget::GLWidget(QWidget *parent)
	: QOpenGLWidget(parent), alpha(0.05), log2_zoom(0),
	  view_type(ViewType::ORTHO), hide_empty_points(true)
{
	QSizePolicy size_policy;
	size_policy.setVerticalPolicy(QSizePolicy::MinimumExpanding);
	size_policy.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
	setSizePolicy(size_policy);
	contours_mode = false;
	hide_above = false;
	hide_below = false;
	highlight = false;
  	color_mode = false;
}

GLWidget::~GLWidget() {}

float GLWidget::getAlpha() const { return alpha; }

void GLWidget::setAlpha(double new_alpha)
{
	alpha = new_alpha;
	update();
}

void GLWidget::onContoursModeChange(int state)
{
	if (state >= 1)
		contours_mode = true;
	else
		contours_mode = false;
	updateDisplayPoints();
	update();
}

void GLWidget::onColorModeChange(int state)
{
	if (state >= 1)
		color_mode = true;
	else
		color_mode = false;
	updateDisplayPoints();
	update();
}

void GLWidget::highlightActiveLayer(int state){
  	if(state == 0)
  	{
		highlight = false;
  	}
	else
	{
		highlight = true;
	}
 	updateDisplayPoints();
  	update();
}

void GLWidget::hideLayersBelow(int state){
	if(state == 0)
		hide_below = false;
	else
		hide_below = true;
	updateDisplayPoints();
	update();
}

void GLWidget::hideLayersAbove(int state){
	if(state == 0)
		hide_above = false;
	else
		hide_above = true;
	updateDisplayPoints();
	update();
}

void GLWidget::saveXYZ() {
	ofstream MyFile("points.xyz");
	for (const DrawablePoint &p : display_points)
	{
		MyFile << p.pos.x() << " " << p.pos.y() << " " << p.pos.z() << "\n";
	}
	MyFile.close();
}

void GLWidget::updateVolumicData(std::unique_ptr<VolumicData> new_data)
{
	volumic_data = std::move(new_data);
	updateDisplayPoints();
	update();
}

void GLWidget::updateDisplayPoints()
{
	display_points.clear();
	if (!volumic_data)
		return;
	int W = volumic_data->width;
	int H = volumic_data->height;
	int D = volumic_data->depth;
	int col = 0;
	int row = 0;
	int depth = 0;
	double x_factor = volumic_data->pixel_width;
	double y_factor = volumic_data->pixel_height;
	double z_factor = volumic_data->slice_spacing;
	double max_size =
		std::max(std::max(x_factor * W, y_factor * H), z_factor * D);
	double global_factor = 2.0 / max_size;
	x_factor *= global_factor;
	y_factor *= global_factor;
	z_factor *= global_factor;
	int idx_start = 0;
	int idx_end = W * H * D;
	if(!hide_below && !hide_above)
	{
		idx_start = 0;
		idx_end = W * H * D;
	}
	else if(hide_below && !hide_above)
	{
		idx_start = W * H * (curr_slice-1);
		idx_end = W * H * D;
	}
	else if(!hide_below && hide_above)
	{
		idx_start = 0;
		idx_end = W * H * (curr_slice);
	}
	else if(hide_above && hide_below)
	{
		idx_start = W * H * (curr_slice-1);
		idx_end = W * H * (curr_slice);
	}

	idx_end = std::min(idx_end, W*H*D);
	int range = idx_end - idx_start;
	if(range <= 0)
		return;
	display_points.reserve(range);

	double cur_win_min;
	double cur_win_max;
	getWinMinMax(&cur_win_min, &cur_win_max);

	// Importing points
	for (int idx = idx_start; idx < idx_end; idx++)
	{
		double raw_color = volumic_data->data[idx];
		double c = volumic_data->manualWindowHandling(raw_color); // c [0;1]

		if (c > 0 || !hide_empty_points)
		{
			int segment = volumic_data->threshold(raw_color, cur_win_min, cur_win_max, color_mode);

			if (segment != 0)
			{
				if (!contours_mode || (contours_mode && connectivity(color_mode ? 0 : 2, idx, segment, cur_win_min, cur_win_max)))
				{
					DrawablePoint p;
					p.a = alpha;
					if(highlight){
      					if(idx>=(curr_slice-1)*W*H && idx<(curr_slice)*W*H)
            				p.a = 1.0;   
    				}
					p.color = volumic_data->getColorSegment(segment, c);

					p.pos = QVector3D((col - W / 2.) * x_factor, (row - H / 2.) * y_factor, (depth - D / 2.) * z_factor);
					display_points.push_back(p);
				}
			}
		}
		col++;
		if (col == W)
		{
			row++;
			col = 0;
		}
		if (row == H)
		{
			depth++;
			row = 0;
		}
	}
	std::cout << "Nb points: " << display_points.size() << std::endl;
}

void GLWidget::initializeGL()
{
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_NEVER);
}

void GLWidget::paintGL()
{
	QSize viewport_size = size();
	int width = viewport_size.width();
	int height = viewport_size.height();
	double aspect_ratio = width / (float)height;
	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	switch (view_type)
	{
	case ViewType::ORTHO:
	{
		double view_half_size = std::pow(2, -log2_zoom);
		glScalef(1.0, aspect_ratio, 1.0);
		QVector3D center(0, 0, 0);
		glOrtho(center.x() - view_half_size, center.x() + view_half_size,
				center.y() - view_half_size, center.y() + view_half_size,
				center.z() - view_half_size, center.z() + view_half_size);
		glMultMatrixf(transform.constData());
		break;
	}
	case ViewType::FRUSTUM:
	{
		float near_dist = 0.5;
		float far_dist = 5.0;
		QMatrix4x4 projection;
		projection.perspective(90, aspect_ratio, near_dist, far_dist);
		QMatrix4x4 cam_offset;
		cam_offset.translate(0, 0, -2 * (1 - log2_zoom));
		QMatrix4x4 pov = projection * cam_offset * transform;
		glMultMatrixf(pov.constData());
	}
	}

	glMatrixMode(GL_MODELVIEW);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glBegin(GL_POINTS);
	for (const DrawablePoint &p : display_points)
	{
		if(highlight && p.a == 1.0)
      		glColor4f(p.color.x(), p.color.y(), p.color.z(), p.a);
    	else 
      		glColor4f(p.color.x(), p.color.y(), p.color.z(), alpha);
    	glVertex3d(p.pos.x(), p.pos.y(), p.pos.z());
	}
	glEnd();
}

void GLWidget::mousePressEvent(QMouseEvent *event) { lastPos = event->pos(); }

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
	double dx = modifiedDelta(event->x() - lastPos.x());
	double dy = modifiedDelta(event->y() - lastPos.y());

	if (event->buttons() & Qt::LeftButton)
	{
		QQuaternion local_rotation =
			QQuaternion::fromEulerAngles(0.5 * dy, 0.5 * dx, 0);
		QMatrix4x4 local_matrix;
		local_matrix.rotate(local_rotation);
		transform = local_matrix * transform;
	}
	else if (event->buttons() & Qt::RightButton)
	{
		QMatrix4x4 local_matrix;
		local_matrix.translate(QVector3D(dx, -dy, 0) * 0.001);
		transform = local_matrix * transform;
	}

	lastPos = event->pos();
	update();
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
	double delta = modifiedDelta(event->delta() / 1000.0);
	log2_zoom += delta;
	update();
}

double GLWidget::modifiedDelta(double delta)
{
	if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
	{
		return 10 * delta;
	}
	return delta;
}

void GLWidget::setWinCenter(double new_value)
{
	win_center = new_value;
	updateDisplayPoints();
	update();
}

void GLWidget::setWinWidth(double new_value)
{
	win_width = new_value;
	updateDisplayPoints();
	update();
}

bool GLWidget::connectivity(const int mode, const int idx, const int curr_segment, const double min, const double max)
{
	const int W = volumic_data->width;
	const int H = volumic_data->height;
	const int D = volumic_data->depth;

	const QVector3D pos = volumic_data->getCoordinate(idx);
	
	const int x = pos.x();
	const int y = pos.y();
	const int z = pos.z();

	for (int dz = -1; dz <= 1; ++dz)
		for (int dy = -1; dy <= 1; ++dy)
			for (int dx = -1; dx <= 1; ++dx)
			{
				int new_x = x + dx;
				int new_y = y + dy;
				int new_z = z + dz;

				if(new_x < 0 || new_y < 0 || new_z < 0 || new_x >= W || new_y >= H || new_z >= D)
					continue;

				switch(mode)
				{
					case 0:
					{
						if(!(abs(dx+dy+dz) == 1 && (dx == 0 || dy == 0 || dz == 0)))
							continue;
						break;
					}
					case 1:
					{
						if(dx != 0 && dy != 0 && dz != 0)
							continue;
						break;
					}
					case 2: break;
					default: break;
				}	

				double raw_color = volumic_data->getValue(new_x, new_y, new_z);
				int neighbor_segment = volumic_data->threshold(raw_color, min, max, color_mode);
				if (curr_segment != neighbor_segment) {
					return true;
				}
			}

	return false;
}

void GLWidget::getWinMinMax(double* min, double* max) {
	if(min)
		*min = win_center - (win_width / 2);
	if(max)
		*max = win_center + (win_width / 2);
}