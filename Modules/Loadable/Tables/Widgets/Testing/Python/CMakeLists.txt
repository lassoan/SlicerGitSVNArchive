if(Slicer_USE_QtTesting AND Slicer_USE_PYTHONQT)
  slicerMacroBuildScriptedModule(
    NAME TablesWidgetsSelfTest
    SCRIPTS TablesWidgetsSelfTest.py
    )
  slicer_add_python_unittest(SCRIPT TablesWidgetsSelfTest.py)
endif()
