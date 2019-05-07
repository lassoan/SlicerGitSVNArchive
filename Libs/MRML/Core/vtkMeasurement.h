/*=auto=========================================================================

Portions (c) Copyright 2017 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

=========================================================================auto=*/

#ifndef __vtkMeasurement_h
#define __vtkMeasurement_h

// MRML includes
#include <vtkCodedEntry.h>
#include "vtkMRML.h"

// VTK includes
#include <vtkObject.h>
#include <vtkSmartPointer.h>

/// \brief Simple class for storing standard coded entries (coding scheme, value, meaning triplets)
///
/// vtkMeasurement is a lightweight class that stores standard coded entries consisting of
/// CodingSchemeDesignator + CodeValue + CodeMeaning triplets.
/// This is a commonly used concept in DICOM, see chapter 3: Encoding of Coded Entry Data
/// (http://dicom.nema.org/medical/dicom/current/output/chtml/part03/sect_8.3.html).
///
class VTK_MRML_EXPORT vtkMeasurement : public vtkObject
{
public:

  static vtkMeasurement *New();
  vtkTypeMacro(vtkMeasurement, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Reset state of object
  virtual void Initialize();

  /// Copy one type into another (deep copy)
  virtual void Copy(vtkMeasurement* aEntry);

  /// Measurement name
  vtkGetStringMacro(Name);
  vtkSetStringMacro(Name);

  /// Measured quantity value
  vtkGetMacro(Value, double);
  vtkSetMacro(Value, double);

  /// Measurement unit
  vtkGetStringMacro(Units);
  vtkSetStringMacro(Units);

  /// Informal description of the measurement
  vtkGetStringMacro(Description);
  vtkSetStringMacro(Description);

  /// Formatting string for displaying measurement value and units
  vtkGetStringMacro(PrintFormat);
  vtkSetStringMacro(PrintFormat);

  // Copies content of coded entry
  void SetQuantityCode(vtkCodedEntry* entry);
  vtkGetObjectMacro(QuantityCode, vtkCodedEntry);

  // Copies content of coded entry
  void SetDerivationCode(vtkCodedEntry* entry);
  vtkGetObjectMacro(DerivationCode, vtkCodedEntry);

  // Copies content of coded entry
  void SetUnitsCode(vtkCodedEntry* entry);
  vtkGetObjectMacro(UnitsCode, vtkCodedEntry);

  // Copies content of coded entry
  void SetMethodCode(vtkCodedEntry* entry);
  vtkGetObjectMacro(MethodCode, vtkCodedEntry);

  ///
  /// Get measurement value and units as a single human-readable string.
  std::string GetValueWithUnitsAsPrintableString();

  ///
  /// Get content of the object as a single machine-readable string.
  std::string GetAsString();

  ///
  /// Set content of the object from a single machine-readable string.
  /// \return true on success
  bool SetFromString(const std::string& content);

protected:
  vtkMeasurement();
  ~vtkMeasurement() override;
  vtkMeasurement(const vtkMeasurement&);
  void operator=(const vtkMeasurement&);

protected:
  char* Name;
  double Value;
  char* Units;
  char* Description;
  char* PrintFormat; // for value (double), unit (string)
  vtkCodedEntry* QuantityCode;   // volume
  vtkCodedEntry* DerivationCode; // min/max/mean
  vtkCodedEntry* UnitsCode;      // cubic millimeter
  vtkCodedEntry* MethodCode;     // Sum of segmented voxel volumes
};

#endif
