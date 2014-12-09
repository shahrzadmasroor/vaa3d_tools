/* cellSegmentation.cpp
 * It aims to automatically segment cells;
 * 2014-10-12 :by Xiang Li (lindbergh.li@gmail.com);
 */
 
#ifndef __CELLSEGMENTATION_PLUGIN_H__
#define __CELLSEGMENTATION_PLUGIN_H__

#pragma region "includes and constants"
#include <QtGui>
#include <v3d_interface.h>
#include <sstream>
#include <math.h>
#include <iostream>
#include <string>
#include "v3d_message.h"
#include "cellSegmentation_plugin.h"
#include <vector>
#include <cassert>
#include <math.h>
#include "string"
#include "sstream"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <basic_landmark.h>
#include "compute_win_pca.h"
#include "convert_type2uint8.h"
using namespace std;
const V3DLONG const_length_histogram = 256;
const double const_max_voxelValue = 255;
const V3DLONG const_count_neighbors = 26; //27 directions -1;
const double default_threshold_global = 35; //a small enough value for the last resort;
const V3DLONG default_threshold_regionSize = 125; //cube of 5 voxels length;
const double const_infinitesimal = 0.000000001;
const V3DLONG const_shapeStatCount = 4;
//const double const_uThresholdMultiplier_regionSize = 0.000125; //0.05*0.05*0.05;
const double const_uThresholdMultiplier_regionSize = 0.1;
const double const_criteria_dominant1 = 0.2;
const double const_criteria_dominant2 = 0.7;
const V3DLONG const_multiplier_uThresholdRegionSize = 15;
const V3DLONG default_max_exemplar=5;
const V3DLONG const_max_learnIteration = 5000;
const V3DLONG const_max_iteration=100;
const V3DLONG const_threshold_centerDistance2 = 36;
#define INF 1E9
#define NINF -1E9
#define PI 3.14159265
enum enum_shape_t {sphere, cube};
#pragma endregion


class cellSegmentation :public QObject, public V3DPluginInterface2_1
{
public:
	Q_OBJECT Q_INTERFACES(V3DPluginInterface2_1);
	float getPluginVersion() const {return 1.1f;}
	QStringList menulist() const;
	void domenu(const QString &menu_name, V3DPluginCallback2 &callback, QWidget *parent);
	QStringList funclist() const ;
	bool dofunc(const QString &func_name, const V3DPluginArgList &input, V3DPluginArgList &output, V3DPluginCallback2 &callback, QWidget *parent);
};

#pragma region "dialogInitialization" 
class dialogRun:public QDialog
{
	Q_OBJECT
public:
	dialogRun(V3DPluginCallback2 &V3DPluginCallback2_currentCallback, QWidget *QWidget_parent, int int_channelDim)
	{
		//channel;
		QStringList QStringList_channel_items;
		if (int_channelDim==1)
		{
			QStringList_channel_items << "1";
		}
		else if (int_channelDim==3)
		{
			QStringList_channel_items << "1 - red";
			QStringList_channel_items << "2 - green";
			QStringList_channel_items << "3 - blue";
		}
		else
		{
			for (int i=1; i<=int_channelDim; i++)
			{
				QStringList_channel_items << QString().setNum(i);
			}
		}
		QComboBox_channel_selection = new QComboBox();
		QComboBox_channel_selection->addItems(QStringList_channel_items);
		if (QStringList_channel_items.size()>1)
		{
			QComboBox_channel_selection->setCurrentIndex(0);
		}
		QGroupBox *QGroupBox_channel_main = new QGroupBox("Color channel");
		QGroupBox_channel_main->setStyle(new QWindowsStyle());
		QGridLayout *QGridLayout_channel_main = new QGridLayout();
		QGroupBox_channel_main->setStyle(new QWindowsStyle());
		QGridLayout_channel_main->addWidget(QComboBox_channel_selection, 1,1,1,1);
		QGroupBox_channel_main->setLayout(QGridLayout_channel_main);
		//exemplar;
		QGroupBox *QGroupBox_exemplar_main = new QGroupBox("Exemplar definition");
		QGridLayout *QGridLayout_exemplar_main = new QGridLayout();
		QLabel* QLabel_exemplar_maxMovement1 = new QLabel(QObject::tr("Max movement from\nmass center"));
		QLineEdit_exemplar_maxMovement1 = new QLineEdit("2", QWidget_parent);
		QLabel* QLabel_exemplar_maxMovement2 = new QLabel(QObject::tr("Max movement from\nmarker position"));
		QLineEdit_exemplar_maxMovement2 = new QLineEdit("3", QWidget_parent);
		QGridLayout_exemplar_main->addWidget(QLabel_exemplar_maxMovement1, 1, 1, 1, 1);
		QGridLayout_exemplar_main->addWidget(QLineEdit_exemplar_maxMovement1, 1, 2, 1, 1);
		QGridLayout_exemplar_main->addWidget(QLabel_exemplar_maxMovement2, 1, 3, 1, 1);
		QGridLayout_exemplar_main->addWidget(QLineEdit_exemplar_maxMovement2, 1, 4, 1, 1);
		QGroupBox_exemplar_main->setLayout(QGridLayout_exemplar_main);
		//shape;
		QGroupBox *QGroupBox_shape_main = new QGroupBox("Geometry stat");
		QGridLayout *QGridLayout_shape_main = new QGridLayout();
		QRadioButton_shape_sphere = new QRadioButton("sphere-like", QWidget_parent);
		QRadioButton_shape_sphere->setChecked(true);
		QRadioButton_shape_cube = new QRadioButton("cube-like", QWidget_parent);
		QRadioButton_shape_cube->setChecked(false);
		QGridLayout_shape_main->addWidget(QRadioButton_shape_sphere, 1, 1, 1, 1);
		QGridLayout_shape_main->addWidget(QRadioButton_shape_cube, 1, 2, 1, 1);
		QLabel* QLabel_shape_delta = new QLabel(QObject::tr("max anisotropic\ndeviation:"));
		QLineEdit_Shape_delta = new QLineEdit("2", QWidget_parent);
		QGridLayout_shape_main->addWidget(QLabel_shape_delta, 1, 3, 1, 1);
		QGridLayout_shape_main->addWidget(QLineEdit_Shape_delta, 1, 4, 1, 1);
		//QLabel* QLabel_shape_thresholdRegionSize = new QLabel(QObject::tr("Min region size\nvs. exemplar ratio:"));
		//QLineEdit_shape_thresholdRegionSize = new QLineEdit("0.1", QWidget_parent);
		QLabel* QLabel_shape_maxExemplar = new QLabel(QObject::tr("# of exemplars per iteration:"));
		QLineEdit_shape_maxExemplar = new QLineEdit(QString("%1").arg(default_max_exemplar), QWidget_parent);
		//QGridLayout_shape_main->addWidget(QLabel_shape_thresholdRegionSize, 2, 1, 1, 1);
		//QGridLayout_shape_main->addWidget(QLineEdit_shape_thresholdRegionSize, 2, 2, 1, 1);
		QGridLayout_shape_main->addWidget(QLabel_shape_maxExemplar, 2, 1, 1, 1);
		QGridLayout_shape_main->addWidget(QLineEdit_shape_maxExemplar, 2, 2, 1, 1);
		QGroupBox_shape_main->setLayout(QGridLayout_shape_main);
		//control;
		QPushButton *QPushButton_control_start = new QPushButton(QObject::tr("Run"));
		QPushButton *QPushButton_control_close = new QPushButton(QObject::tr("Close"));
		QWidget* QWidget_control_bar = new QWidget();
		QGridLayout* QGridLayout_control_bar = new QGridLayout();
		QGridLayout_control_bar->addWidget(QPushButton_control_start,1,1,1,1);
		QGridLayout_control_bar->addWidget(QPushButton_control_close,1,2,1,1);
		QWidget_control_bar->setLayout(QGridLayout_control_bar);
		//main panel;
		QGridLayout *QGridLayout_main = new QGridLayout();
		QGridLayout_main->addWidget(QGroupBox_channel_main);
		QGridLayout_main->addWidget(QGroupBox_shape_main);
		QGridLayout_main->addWidget(QGroupBox_exemplar_main);
		QGridLayout_main->addWidget(QWidget_control_bar);
		setLayout(QGridLayout_main);
		setWindowTitle(QString("cellSegmentation: quickFind"));
		//event evoking;
		connect(QPushButton_control_start, SIGNAL(clicked()), this, SLOT(_slot_start()));
		connect(QPushButton_control_close, SIGNAL(clicked()), this, SLOT(reject()));
		update();
	}
	~dialogRun(){}
	QComboBox* QComboBox_channel_selection;
	V3DLONG channel_idx_selection;
	QLineEdit* QLineEdit_Shape_delta;
	//QLineEdit* QLineEdit_shape_thresholdRegionSize;
	QLineEdit* QLineEdit_shape_maxExemplar;
	QLineEdit* QLineEdit_exemplar_maxMovement1;
	QLineEdit* QLineEdit_exemplar_maxMovement2;
	QRadioButton* QRadioButton_shape_sphere;
	QRadioButton* QRadioButton_shape_cube;
	enum_shape_t shape_type_selection;
	double shape_para_delta;
	//double shape_multiplier_thresholdRegionSize;
	V3DLONG shape_max_exempalr;
	V3DLONG exemplar_maxMovement1;
	V3DLONG exemplar_maxMovement2;
	public slots:
	void _slot_start()
	{
		channel_idx_selection = QComboBox_channel_selection->currentIndex() + 1;
		shape_para_delta = this->QLineEdit_Shape_delta->text().toDouble();
		//shape_multiplier_thresholdRegionSize = this->QLineEdit_shape_thresholdRegionSize->text().toDouble();
		shape_max_exempalr = this->QLineEdit_shape_maxExemplar->text().toUInt();
		exemplar_maxMovement1 = this->QLineEdit_exemplar_maxMovement1->text().toUInt();
		exemplar_maxMovement2 = this->QLineEdit_exemplar_maxMovement2->text().toUInt();
		if (this->QRadioButton_shape_sphere->isChecked())
		{
			this->shape_type_selection = sphere;
		}
		else if (this->QRadioButton_shape_cube->isChecked())
		{
			this->shape_type_selection = cube;
		}
		accept();
	}
};
#pragma endregion






#endif