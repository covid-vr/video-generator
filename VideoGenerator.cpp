#include "VideoGenerator.h"

#include "QmitkAnimationWidget.h"
#include "QmitkAnimationItemDelegate.h"
#include "QmitkOrbitAnimationItem.h"
#include "QmitkOrbitAnimationWidget.h"
#include "QmitkRenderWindow.h"
#include "QmitkSliceWidget.h"

#include "QmitkFFmpegWriter2.h"
#include <mitkRenderingManager.h>
#include <mitkBaseRenderer.h>

#include "mitkProperties.h"
#include "mitkImageAccessByItk.h"
#include <mitkIOUtil.h>

#include <mitkTransferFunction.h>
#include <mitkTransferFunctionProperty.h>
#include <mitkTransferFunctionPropertySerializer.h>

#include "vtkRenderer.h"
#include "vtkImageWriter.h"
#include "vtkJPEGWriter.h"
#include "vtkPNGWriter.h"
#include "vtkRenderLargeImage.h"
#include <vtkCamera.h>
#include <QString>
#include <QDir>


VideoGenerator::VideoGenerator(int argc, char *argv[]){
  Load(argc, argv);
}


VideoGenerator::~VideoGenerator(){
}


void VideoGenerator::Initialize(){
	//Setting the data
	mitk::StandaloneDataStorage::SetOfObjects::ConstPointer dataNodes = this->dataStorageOriginal->GetAll();

	//Reading each image and setting TransferFunction 
	for(int image_index = 0; image_index < dataNodes->Size(); image_index++) {
		mitk::Image::Pointer currentMitkImage = dynamic_cast<mitk::Image *>(dataNodes->at(image_index)->GetData());

		this->resultNode = mitk::DataNode::New();
		this->dataStorageModified->Add(this->resultNode);
		this->resultNode->SetData(currentMitkImage);
		this->resultNode->SetProperty("binary", mitk::BoolProperty::New(false));

		// Volume Rendering and Transfer function
		this->resultNode->SetProperty("volumerendering", mitk::BoolProperty::New(true));
		this->resultNode->SetProperty("volumerendering.usegpu", mitk::BoolProperty::New(true));
		this->resultNode->SetProperty("layer", mitk::IntProperty::New(1));

		mitk::TransferFunction::Pointer tf = mitk::TransferFunction::New();
		tf->InitializeByMitkImage(currentMitkImage);
		tf = mitk::TransferFunctionPropertySerializer::DeserializeTransferFunction(this->transferFunctionFile.toLatin1());

		this->resultNode->SetProperty("TransferFunction", mitk::TransferFunctionProperty::New(tf.GetPointer()));
		mitk::LevelWindowProperty::Pointer levWinProp = mitk::LevelWindowProperty::New();
		mitk::LevelWindow levelwindow;
		levelwindow.SetAuto(currentMitkImage);
		levWinProp->SetLevelWindow(levelwindow);
		this->resultNode->SetProperty("levelwindow", levWinProp);
	}

	this->InitializeRenderWindow();

}


void ReadPixels(std::unique_ptr<unsigned char[]>& frame, vtkRenderWindow* renderWindow, int x, int y, int width, int height) { 
	if(nullptr == renderWindow) {
		return;
	}
	renderWindow->MakeCurrent();
	glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, frame.get());
}


QmitkAnimationItem* CreateDefaultAnimation(const QString& widgetKey, double time) {
	if (widgetKey == "Orbit"){
		return new QmitkOrbitAnimationItem(360, false, time, 0.0, false);
	}
	return nullptr;
}


void VideoGenerator::CalculateTotalDuration(int fpsSpinBox) {
	const int rowCount = animationModel->rowCount();
	double totalDuration = 0.0;
	double previousStart = 0.0;

	for(int i = 0; i < rowCount; ++i) {
		auto item = dynamic_cast<QmitkAnimationItem*>(animationModel->item(i, 1));
		if(nullptr == item) {
			continue;
		}

		if(item->GetStartWithPrevious()) {
			totalDuration = std::max(totalDuration, previousStart + item->GetDelay() + item->GetDuration());
		} else {
			previousStart = totalDuration;
			totalDuration += item->GetDelay() + item->GetDuration();
		}
	}

	this->totalDuration = totalDuration;
	this->numFrames = static_cast<int>(totalDuration * fpsSpinBox);
	// cout<<"Duration: "<<totalDuration<<", Frames"<<numFrames<<endl;
}


void VideoGenerator::InitializeRenderWindow() {
	mitk::StandaloneDataStorage::Pointer ds = mitk::StandaloneDataStorage::New();
	QmitkRenderWindow* renderWindow = new QmitkRenderWindow();

	// Tell the renderwindow which (part of) the tree to render
	renderWindow->GetRenderer()->SetDataStorage(dataStorageModified);

	// Use it as a 3D view
	renderWindow->GetRenderer()->SetMapperID(mitk::BaseRenderer::Standard3D);
	renderWindow->resize(this->width+6, this->height+6);

	// Do not display
	renderWindow->setAttribute(Qt::WA_DontShowOnScreen);
	renderWindow->show();


	renderWindow2 = renderWindow->GetVtkRenderWindow();
	mitk::BaseRenderer* baserenderer = mitk::BaseRenderer::GetInstance(renderWindow2);
	auto renderer = baserenderer->GetVtkRenderer();

	double bgcolor[] = {this->backgroundColor.red()/255.0, this->backgroundColor.green()/255.0, this->backgroundColor.blue()/255.0};
	renderer->SetBackground(bgcolor);


	renderer->ResetCamera();
	vtkCamera* vtkcam = renderer->GetActiveCamera();
	vtkcam->Zoom(1.5);

	vtkcam->Roll( 90 );
	vtkcam->Azimuth( 90 );
	vtkcam->Roll( -90 );

	if(nullptr == renderWindow){
		return;
	}
	int fpsSpinBox = this->fps;

	fFmpegWriter = new QmitkFFmpegWriter2(); 

	const QString ffmpegPath =  "/usr/bin/ffmpeg";
	fFmpegWriter->SetFFmpegPath(ffmpegPath);

	const int border = 3;
	const int x = border;
	const int y = border;
	int width = renderWindow2->GetSize()[0] - border * 2; 
	int height = renderWindow2->GetSize()[1] - border * 2; 

	if (width & 1) {
		--width;
	}
	if (height & 1) {
		--height;
	}
	if (width < 16 || height < 16) {
		return;
	}

	fFmpegWriter->SetSize(width, height);
	fFmpegWriter->SetFramerate(fpsSpinBox);

	QString saveFileName = this->outputDir;
	if(!saveFileName.endsWith(".mp4")) {
		saveFileName += ".mp4";
	}

	fFmpegWriter->SetOutputPath(saveFileName);

	//Animation
	animationModel = new QStandardItemModel(nullptr);
	const auto key = "Orbit";
	animationModel->appendRow(QList<QStandardItem*>() << new QStandardItem(key) << CreateDefaultAnimation(key, this->time));

	this->CalculateTotalDuration(fpsSpinBox);

	try {
		auto frame = std::make_unique<unsigned char[]>(width * height * 3);
		fFmpegWriter->Start();

		for(this->currentFrame = 0; this->currentFrame < numFrames; ++this->currentFrame) {
			this->RenderCurrentFrame();
			ReadPixels(frame, renderWindow2, x, y, width, height);
			fFmpegWriter->WriteFrame(frame.get());
		}
		fFmpegWriter->Stop();

		this->currentFrame = 0;
		this->RenderCurrentFrame();
	}
	catch(const mitk::Exception& exception) {
		cout<< "Movie Maker: "<< exception.GetDescription()<<endl;
	}
}


void VideoGenerator::RenderCurrentFrame() {
	double deltaT = this->totalDuration / (this->numFrames - 1);
	const auto activeAnimations = this->GetActiveAnimations(this->currentFrame * deltaT);

	for(const auto& animation : activeAnimations) {
		const auto nextActiveAnimations = this->GetActiveAnimations((this->currentFrame + 1) * deltaT);
		bool lastFrameForAnimation = true;
		
		for(const auto& nextAnimation : nextActiveAnimations) {
			if (nextAnimation.first == animation.first){
				lastFrameForAnimation = false;
				break;
			}	
		}
		
		animation.first->Animate2(!lastFrameForAnimation ? animation.second : 1.0, renderWindow2);
	}
	mitk::RenderingManager::GetInstance()->ForceImmediateUpdateAll();
}


std::vector<std::pair<QmitkAnimationItem*, double>> VideoGenerator::GetActiveAnimations(double t) const {
	const int rowCount = this->animationModel->rowCount();
	std::vector<std::pair<QmitkAnimationItem*, double>> activeAnimations;

	double totalDuration = 0.0;
	double previousStart = 0.0;
	for(int i = 0; i < rowCount; ++i) {
		QmitkAnimationItem* item = dynamic_cast<QmitkAnimationItem*>(this->animationModel->item(i, 1));
		if(item == nullptr) {
			continue;
		}
		
		if(item->GetDuration() > 0.0) {
			double start = item->GetStartWithPrevious() ? previousStart + item->GetDelay() : totalDuration + item->GetDelay();
			if (start <= t && t <= start + item->GetDuration()) {
				activeAnimations.emplace_back(item, (t - start) / item->GetDuration());
			}
		}

		if (item->GetStartWithPrevious()){
			totalDuration = std::max(totalDuration, previousStart + item->GetDelay() + item->GetDuration());
		} else {
			previousStart = totalDuration;
			totalDuration += item->GetDelay() + item->GetDuration();
		}
	}
	return activeAnimations;
}


void VideoGenerator::Load(int argc, char *argv[]) {

	this->dataStorageOriginal = mitk::StandaloneDataStorage::New();
	this->dataStorageModified = mitk::StandaloneDataStorage::New();

	int i = 1;
	while(i < argc) {
		if(strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--image") == 0) {
			mitk::StandaloneDataStorage::SetOfObjects::Pointer dataNodes = mitk::IOUtil::Load(argv[i + 1], *this->dataStorageOriginal);
			if(dataNodes->empty()) {
				fprintf(stderr, "Could not open file %s \n\n", argv[i + 1]);
				exit(2);
			}
		}
		if(strcmp(argv[i], "-tf") == 0 || strcmp(argv[i], "--transfer_function") == 0){
			this->transferFunctionFile=argv[i + 1];
		}
		if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0){
			this->outputDir=argv[i + 1];
		}
		if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--width") == 0){
			this->width=atoi(argv[i + 1]);
		}
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--height") == 0){
			this->height=atoi(argv[i + 1]);
		}
		if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--time") == 0){
			this->time=atof(argv[i + 1]);
		}
		if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--frames") == 0){
			this->fps=atoi(argv[i + 1]);
		}
		if(strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--color") == 0){
			QString color = argv[i + 1];
			QStringList colorList = color.split(",");

			int rgb[3] = {255};
			for(int c = 0; c < colorList.length(); c++) {
				if(c > 2) {
					break;
				}
				int value = colorList.value(c).toInt();
				if (value > -1 && value < 256) {
					rgb[c] = value;
				}
			}
			this->backgroundColor = QColor(rgb[0], rgb[1], rgb[2]);
		}
		i += 2;
	}
}

