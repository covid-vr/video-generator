#include "QmitkRegisterClasses.h"
#include "VideoGenerator.h"

#include "mitkDataStorage.h"

#include <QApplication>
#include <itksys/SystemTools.hxx>


int main(int argc, char *argv[]) {
	QApplication qtapplication(argc, argv);

	if(argc < 2){
		fprintf(stderr, "Usage:   %s -i [filename1] [filename2] ... -tf [filename3]\n\n", itksys::SystemTools::GetFilenameName(argv[0]).c_str());
		return 1;
	}

	QmitkRegisterClasses();

	VideoGenerator mainWidget(argc, argv);
	mainWidget.Initialize();

	return 0;
}
