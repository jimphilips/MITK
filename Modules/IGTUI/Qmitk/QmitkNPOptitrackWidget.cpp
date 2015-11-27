/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#include "QmitkNPOptitrackWidget.h"
#include "QmitkTrackingDeviceConfigurationWidget.h"

#include <QFileDialog>
#include <QMessageBox>
#include <Poco/Path.h>

#include <mitkOptitrackTrackingDevice.h>

const std::string QmitkNPOptitrackWidget::VIEW_ID = "org.mitk.views.NPOptitrackWidget";

QmitkNPOptitrackWidget::QmitkNPOptitrackWidget(QWidget* parent, Qt::WindowFlags f)
  : QmitkAbstractTrackingDeviceWidget(parent, f)
{
  m_Controls = NULL;
  CreateQtPartControl(this);
  CreateConnections();
  m_ErrorMessage = "";
}

QmitkNPOptitrackWidget::~QmitkNPOptitrackWidget()
{
}

void QmitkNPOptitrackWidget::CreateQtPartControl(QWidget *parent)
{
  if (!m_Controls)
  {
    // create GUI widgets
    m_Controls = new Ui::QmitkNPOptitrackWidget;
    m_Controls->setupUi(parent);
  }
}

void QmitkNPOptitrackWidget::CreateConnections()
{
  if (m_Controls)
  {
    connect((QObject*)(m_Controls->m_testConnectionOptitrack), SIGNAL(clicked()), this, SLOT(TestConnection()));
    connect((QObject*)(m_Controls->m_SetOptitrackCalibrationFile), SIGNAL(clicked()), this, SLOT(SetOptitrackCalibrationFileClicked()));
  }
}

void QmitkNPOptitrackWidget::ResetOutput()
{
  m_Controls->m_outputTextOptitrack->setHtml("<body style=\" font-family:\'MS Shell Dlg 2\'; font-size:7pt; font-weight:400; font-style:normal;\" bgcolor=black><span style=\"color:#ffffff;\"><u>output:</u>");
}

void QmitkNPOptitrackWidget::AddOutput(std::string s)
{
  m_Controls->m_outputTextOptitrack->setHtml(QString(s.c_str()));
  m_Controls->m_outputTextOptitrack->verticalScrollBar()->setValue(m_Controls->m_outputTextOptitrack->verticalScrollBar()->maximum());
}

mitk::TrackingDevice::Pointer QmitkNPOptitrackWidget::ConstructTrackingDevice()
{
  // Create the Tracking Device
  mitk::OptitrackTrackingDevice::Pointer tempTrackingDevice = mitk::OptitrackTrackingDevice::New();
  // Set the calibration File
  tempTrackingDevice->SetCalibrationPath(m_OptitrackCalibrationFile);

  //Set the camera parameters
  tempTrackingDevice->SetExp(m_Controls->m_OptitrackExp->value());
  tempTrackingDevice->SetLed(m_Controls->m_OptitrackLed->value());
  tempTrackingDevice->SetThr(m_Controls->m_OptitrackThr->value());

  tempTrackingDevice->SetType(mitk::TRACKING_DEVICE_IDENTIFIER_OPTITRACK);
  return static_cast<mitk::TrackingDevice::Pointer>(tempTrackingDevice);
}

bool QmitkNPOptitrackWidget::IsDeviceInstalled()
{
  return mitk::OptitrackTrackingDevice::New()->IsDeviceInstalled();
}

void QmitkNPOptitrackWidget::SetOptitrackCalibrationFileClicked()
{
  std::string filename = QFileDialog::getOpenFileName(NULL, tr("Open Calibration File"), "/", "*.*").toLatin1().data();
  if (filename == "") { return; }
  else
  {
    m_OptitrackCalibrationFile = filename;
    Poco::Path myPath = Poco::Path(m_OptitrackCalibrationFile.c_str());
    m_Controls->m_OptitrackCalibrationFile->setText("Calibration File: " + QString(myPath.getFileName().c_str()));
  }
}