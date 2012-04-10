#ifndef __MAPVIEW_GUI_H__
#define __MAPVIEW_GUI_H__

#include <iostream>
#include <QtGui>
#include <v3d_interface.h>
#include "mapview.h"

using namespace std;

struct Mapview_Paras {
	int L, M, N, l, m, n;//block
	int level_num;
	int channel;
	int level;           //current level
	V3DLONG outsz[4];    //output size
	V3DLONG origin[3];   //top-left corner pos
	QString hraw_dir; //prefix of files
	bool is_use_thread;

	Mapview_Paras()
	{
		hraw_dir=QString("");
		outsz[0]=outsz[1]=outsz[2]=outsz[3]=0;
		origin[0]=origin[1]=origin[2]=0;
		L = M = N = l = m = n = 0;
		level_num = 0;
		level=0;
		is_use_thread = 1;
	}
};

class MapViewWidget : public QWidget
{
	Q_OBJECT;

public:
	MapViewWidget(V3DPluginCallback2 * _callback, Mapview_Paras _mapview_paras,  QWidget *parent = 0);
	~MapViewWidget(){}
	void updateLevels(int level);
	void updateTriView();
	void closeEvent(QCloseEvent *event);

private:
	ImageMapView mapview;
	Mapview_Paras mapview_paras;
	V3DPluginCallback2 * callback;
	v3dhandle curwin;

	QScrollBar *xSlider_mapv;
	QScrollBar *ySlider_mapv;
	QScrollBar *zSlider_mapv;
	QScrollBar *zoomSlider_mapv;

	QSpinBox *xValueSpinBox_mapv;
	QSpinBox *yValueSpinBox_mapv;
	QSpinBox *zValueSpinBox_mapv;
	QSpinBox *zoomSpinBox_mapv;

	QScrollBar *cropXSlider_mapv;
	QScrollBar *cropYSlider_mapv;
	QScrollBar *cropZSlider_mapv;

	QSpinBox *cropXSpinBox_mapv;
	QSpinBox *cropYSpinBox_mapv;
	QSpinBox *cropZSpinBox_mapv;

	QCheckBox * threadCheckBox;
public slots:
	void changeXOffset_mapv(int x);
	void changeYOffset_mapv(int y);
	void changeZOffset_mapv(int z);
	void changeLevel_mapv(int level);
	void changeWINSZ_mapv(int sz);
	void setMultiThreads(bool is_multi_threads);
};

#endif 
