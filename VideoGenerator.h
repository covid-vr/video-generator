#ifndef VIDEOGENERATOR_H
#define VIDEOGENERATOR_H

#include <mitkImage.h>
#include <mitkPointSet.h>
#include <mitkStandaloneDataStorage.h>

#include "vtkRenderer.h"
#include <mitkTransferFunction.h>
#include <vtkRenderWindow.h>

#include <itkImage.h>
#include <QString>
#include <QColor>

#include <QProcess>

#ifndef DOXYGEN_IGNORE


class QmitkAnimationItem;
class QmitkAnimationWidget;
class QmitkFFmpegWriter2;
class QMenu;
class QStandardItemModel;
class QTimer;
class QItemSelection;


class VideoGenerator {
	public:
		VideoGenerator(int argc, char *argv[]);
		~VideoGenerator();
		virtual void Initialize();

	protected:
		void Load(int argc, char *argv[]);
		virtual void InitializeRenderWindow();

		mitk::StandaloneDataStorage::Pointer dataStorageOriginal;
		mitk::StandaloneDataStorage::Pointer dataStorageModified;

		mitk::DataNode::Pointer resultNode;

		QColor backgroundColor;

		QString           transferFunctionFile;
		QString           outputDir;   
		QString           lastFile;
		QString           PNGExtension = "PNG File (*.png)";
		QString           JPGExtension = "JPEG File (*.jpg)";

	private:
		void SetDefaultTransferFunction(mitk::TransferFunction::Pointer tf);
		void TakeScreenshot(vtkRenderer* renderer, unsigned int magnificationFactor, QString fileName, QString filter = "", QColor backgroundColor = QColor(255,255,255));
		void CalculateTotalDuration(int fpsSpinBox);
		void RenderCurrentFrame();
		std::vector<std::pair<QmitkAnimationItem*, double>> GetActiveAnimations(double t) const;


		QmitkFFmpegWriter2* fFmpegWriter;
		QStandardItemModel* animationModel;
		std::map<QString, QmitkAnimationWidget*> animationWidgets;
		QTimer* timer;
		double totalDuration;
		int numFrames;
		int currentFrame;
		int fps = 60;
		double time =2.0;
		int width = 512;
		int height = 512;
		vtkRenderWindow* renderWindow2;
		QProcess *process;

};

#endif
#endif

