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

#ifndef QmitkMeanSquaresHistogramMetricViewWidgetHIncluded
#define QmitkMeanSquaresHistogramMetricViewWidgetHIncluded

#include "MitkRigidRegistrationUIExports.h"
#include "QmitkRigidRegistrationMetricsGUIBase.h"
#include "ui_QmitkMeanSquaresHistogramMetricControls.h"
#include <itkArray.h>
#include <itkImage.h>
#include <itkObject.h>

/*!
* \brief Widget for rigid registration
*
* Displays options for rigid registration.
*/
class MITKRIGIDREGISTRATIONUI_EXPORT QmitkMeanSquaresHistogramMetricView : public QmitkRigidRegistrationMetricsGUIBase
{
public:
  QmitkMeanSquaresHistogramMetricView(QWidget *parent = nullptr, Qt::WindowFlags f = nullptr);
  ~QmitkMeanSquaresHistogramMetricView();

  virtual mitk::MetricParameters::MetricType GetMetricType() override;

  virtual itk::Object::Pointer GetMetric() override;

  virtual itk::Array<double> GetMetricParameters() override;

  virtual void SetMetricParameters(itk::Array<double> metricValues) override;

  virtual QString GetName() override;

  virtual void SetupUI(QWidget *parent) override;

  virtual bool Maximize() override;

private:
  template <class TPixelType, unsigned int VImageDimension>
  itk::Object::Pointer GetMetric2(itk::Image<TPixelType, VImageDimension> *itkImage1);

protected:
  Ui::QmitkMeanSquaresHistogramMetricControls m_Controls;

  itk::Object::Pointer m_MetricObject;
};

#endif
