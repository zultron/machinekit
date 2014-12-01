import logging
from . import Coprocess

class RTAPIApp(Coprocess):

    args = [ 'rtapi:0' ]
    executable = './fake_rtapi_app.sh'

    log = logging.getLogger(__name__)
