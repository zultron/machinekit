# Make main objects available from machinekit.rtapi module
from rtapi import RTAPI, RTAPIPermissionError
from config import Config, RTAPIConfigException, RTAPIConfigNotFoundException
from environment import Environment, RTAPIEnvironmentInitError, \
    RTAPIEnvironmentRLimitError, RTAPIEnvironmentPrivilegeError
from shm import MKSHMSegment, SHMDrvAPIRuntimeError, \
    SHMOps, RTAPISHMRuntimeError
from util import Util
from rtapi_global import GlobalData, RTAPIGlobalDataException

rtapi_exceptions = (
    RTAPIPermissionError,
    RTAPIConfigException,
    RTAPIConfigNotFoundException,
    RTAPIEnvironmentInitError,
    RTAPIEnvironmentRLimitError,
    RTAPIEnvironmentPrivilegeError,
    SHMDrvAPIRuntimeError,
    RTAPISHMRuntimeError,
    RTAPIGlobalDataException,
    )


# FIXME trash?
from compat import Compat
from setuid_helper import SetuidHelper
