""" This module contains functions for writing info or error messages to the application log."""

import logging
import sys
from __main__ import slicer, ctk, qt

__all__ = ['initLogging']

#-----------------------------------------------------------------------------
class _LogReverseLevelFilter(logging.Filter):
  """Rejects log records that are at or above the specified level
  """
  def __init__(self, levelLimit):
    self._levelLimit = levelLimit
  def filter(self, record):
    return record.levelno < self._levelLimit

class SlicerApplicationLogHandler(logging.Handler):
  """Writes logging records to Slicer application log.
  """
  def emit(self, record):
    try:
      pythonToCtkLevelConverter = {
        logging.DEBUG : ctk.ctkErrorLogLevel.Debug,
        logging.INFO : ctk.ctkErrorLogLevel.Info,
        logging.WARNING : ctk.ctkErrorLogLevel.Warning,
        logging.ERROR : ctk.ctkErrorLogLevel.Error }
      #msg = self.format(record)
      #print "Message: {0}".format(record.msg)
      slicer.app.addLogEntry(pythonToCtkLevelConverter[record.levelno], record.msg, qt.QDateTime.currentDateTime(),
        "{0}({1})".format(record.threadName, record.thread), "Python",
        record.lineno, record.pathname, record.funcName, "myMessage", "myCategory")
    except:
      self.handleError(record)

#-----------------------------------------------------------------------------
def initLogging(logger):
  """Initialize logging.
  """

  # Create log output stream handlers
  #applicationLogHandler = logging.StreamHandler(sys.stdout)
  applicationLogHandler = SlicerApplicationLogHandler()
  applicationLogHandler.setLevel(logging.DEBUG)
  # Filter messages at INFO level or above (they will be printed on the console)
  applicationLogHandler.addFilter(_LogReverseLevelFilter(logging.INFO))
  #applicationLogHandler.setFormatter(f)
  applicationLogHandler.setFormatter(logging.Formatter('%(levelname)s: %(message)s (%(pathname)s:%(lineno)d)'))


  # Prints info message to stdout. This will also show up in the application log.
  consoleInfoHandler = logging.StreamHandler(sys.stdout)
  consoleInfoHandler.setLevel(logging.INFO)
  # Filter messages at WARNING level or above (they will be printed on stderr)
  consoleInfoHandler.addFilter(_LogReverseLevelFilter(logging.WARNING))
  #consoleInfoHandler.setFormatter(f)

  # Prints error and warning messages to stderr. This will also show it in the application log.
  consoleErrorHandler = logging.StreamHandler(sys.stderr)
  consoleErrorHandler.setLevel(logging.WARNING)
  #consoleErrorHandler.setFormatter(f)

  logger.addHandler(applicationLogHandler)
  logger.addHandler(consoleInfoHandler)
  logger.addHandler(consoleErrorHandler)

  logger.setLevel(logging.DEBUG)

#-----------------------------------------------------------------------------
# Set up root 'slicer' logger
logger = slicer.logger
initLogging(logger)
