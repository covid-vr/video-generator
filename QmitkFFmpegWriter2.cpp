#include "QmitkFFmpegWriter2.h"
#include<iostream>
using namespace std;

QmitkFFmpegWriter2::QmitkFFmpegWriter2()
  : QObject(), m_Process(new QProcess(this)), m_Framerate(0), m_IsRunning(false)
{
}

QmitkFFmpegWriter2::~QmitkFFmpegWriter2()
{
}

QString QmitkFFmpegWriter2::GetFFmpegPath() const
{
  return m_FFmpegPath;
}

void QmitkFFmpegWriter2::SetFFmpegPath(const QString &path)
{
  m_FFmpegPath = path;
}

QSize QmitkFFmpegWriter2::GetSize() const
{
  return m_Size;
}

void QmitkFFmpegWriter2::SetSize(const QSize &size)
{
  m_Size = size;
}

void QmitkFFmpegWriter2::SetSize(int width, int height)
{
  m_Size = QSize(width, height);
}

int QmitkFFmpegWriter2::GetFramerate() const
{
  return m_Framerate;
}

void QmitkFFmpegWriter2::SetFramerate(int framerate)
{
  m_Framerate = framerate;
}

QString QmitkFFmpegWriter2::GetOutputPath() const
{
  return m_OutputPath;
}

void QmitkFFmpegWriter2::SetOutputPath(const QString &path)
{
  m_OutputPath = path;
}

void QmitkFFmpegWriter2::Start()
{
  if (m_FFmpegPath.isEmpty())
    cout << "FFmpeg/Libav path is empty!";

  if (m_Size.isNull())
    cout << "Invalid video frame size!";

  if (m_Framerate <= 0)
    cout << "Invalid framerate!";

  if (m_OutputPath.isEmpty())
    cout << "Output path is empty!";

  m_Process->start(m_FFmpegPath,
                   QStringList() << "-y"
                                 << "-f"
                                 << "rawvideo"
                                 << "-pix_fmt"
                                 << "rgb24"
                                 << "-s"
                                 << QString("%1x%2").arg(m_Size.width()).arg(m_Size.height())
                                 << "-r"
                                 << QString("%1").arg(m_Framerate)
                                 << "-i"
                                 << "-"
                                 << "-vf"
                                 << "vflip"
                                 << "-pix_fmt"
                                 << "yuv420p"
                                 << "-crf"
                                 << "18"
                                 << m_OutputPath);

  m_Process->waitForStarted();
  m_IsRunning = true;
}

bool QmitkFFmpegWriter2::IsRunning() const
{
  return m_IsRunning;
}

void QmitkFFmpegWriter2::WriteFrame(const unsigned char *frame)
{
  if (frame == nullptr || !m_Process->isOpen()){
    return;
  }
  m_Process->write(reinterpret_cast<const char *>(frame), m_Size.width() * m_Size.height() * 3);
  m_Process->waitForBytesWritten();
}

void QmitkFFmpegWriter2::Stop()
{
  m_Process->waitForFinished();
  m_IsRunning = false;
  m_Process->closeWriteChannel();
}

void QmitkFFmpegWriter2::OnProcessError(QProcess::ProcessError error)
{
  m_IsRunning = false;

  switch (error)
  {
    case QProcess::FailedToStart:
      std::cout << "FFmpeg/Libav failed to start!";

    case QProcess::Crashed:
      std::cout << "FFmpeg/Libav crashed!";

    case QProcess::Timedout:
      std::cout << "FFmpeg/Libav timed out!";

    case QProcess::WriteError:
      std::cout << "Could not write to FFmpeg/Libav!";

    case QProcess::ReadError:
      std::cout << "Could not read from FFmpeg/Libav!";

    default:
      std::cout << "An unknown error occurred!";
  }
}

void QmitkFFmpegWriter2::OnProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  m_IsRunning = false;

  if (exitStatus != QProcess::CrashExit)
  {
    if (exitCode != 0)
    {
      m_Process->close();
      std::cout << QString("FFmpeg/Libav exit code: %1").arg(exitCode).toStdString().c_str();
    }
  }

  m_Process->close();
}
