transformYmin=-100
transformYmax=100
transformYinc=5
m=vtk.vtkMatrix4x4()

slicer.t1=slicer.vtkMRMLTransformNode()
slicer.t1UpdatePeriod=3
slicer.t1UpdateCounter=0
slicer.t1.SetMatrixTransformToParent(m)
slicer.mrmlScene.AddNode(slicer.t1)

slicer.t2=slicer.vtkMRMLTransformNode()
slicer.t2UpdatePeriod=25
slicer.t2UpdateCounter=0
slicer.t2.SetMatrixTransformToParent(m)
slicer.mrmlScene.AddNode(slicer.t2)

slicer.t3=slicer.vtkMRMLTransformNode()
slicer.t3.SetMatrixTransformToParent(m)
slicer.mrmlScene.AddNode(slicer.t3)

def transformUpdate():
  global transformYinc
  qt.QTimer.singleShot(100, transformUpdate)
  if slicer.t1UpdateCounter>=slicer.t1UpdatePeriod:
    slicer.t1UpdateCounter=0
    m=vtk.vtkMatrix4x4()
    slicer.t1.GetMatrixTransformToParent(m)
    oldElem=m.GetElement(1, 3)
    oldElem=oldElem+transformYinc
    m.SetElement(1, 3, oldElem)
    slicer.t1.SetMatrixTransformToParent(m)
    if oldElem > transformYmax or oldElem < transformYmin:
      transformYinc=-transformYinc
  else:
    slicer.t1UpdateCounter = slicer.t1UpdateCounter + 1
  if slicer.t2UpdateCounter>=slicer.t2UpdatePeriod:
    slicer.t2UpdateCounter=0
    m=vtk.vtkMatrix4x4()
    slicer.t2.GetMatrixTransformToParent(m)
    oldElem=m.GetElement(1, 3)
    oldElem=oldElem+transformYinc
    m.SetElement(1, 3, oldElem)
    slicer.t2.SetMatrixTransformToParent(m)
    if oldElem > transformYmax or oldElem < transformYmin:
      transformYinc=-transformYinc
  else:
    slicer.t2UpdateCounter = slicer.t2UpdateCounter + 1

transformUpdate()
