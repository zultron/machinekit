import os
import sys
from time import *
import subprocess
import signal
from machinekit import compat

_processes = []


# ends a running Machinekit session
def end_session():
    stop_processes()
    stop_realtime()


# checks wheter a single command is available or not
def check_command(command):
    process = subprocess.Popen('which ' + command, stdout=subprocess.PIPE,
                               shell=True)
    process.wait()
    if process.returncode != 0:
        print((command + ' not found, check Machinekit installation'))
        sys.exit(1)


# checks the whole Machinekit installation
def check_installation():
    commands = ['realtime', 'configserver', 'halcmd', 'haltalk', 'webtalk']
    for command in commands:
        check_command(command)


# checks for a running session and cleans it up if necessary
def cleanup_session():
    pids = []
    commands = ['configserver', 'halcmd', 'haltalk', 'webtalk', 'rtapi']
    process = subprocess.Popen(['ps', '-A'], stdout=subprocess.PIPE)
    out, err = process.communicate()
    for line in out.splitlines():
        for command in commands:
            if command in line:
                pid = int(line.split(None, 1)[0])
                pids.append(pid)

    if pids != []:
        sys.stdout.write("cleaning up leftover session... ")
        sys.stdout.flush()
        subprocess.check_call('realtime stop', shell=True)
        for pid in pids:
            try:
                os.kill(pid, signal.SIGTERM)
            except OSError:
                pass
        sys.stdout.write('done\n')


# starts and registers a process
def start_process(command):
    sys.stdout.write("starting " + command.split(None, 1)[0] + "... ")
    sys.stdout.flush()
    process = subprocess.Popen(command, shell=True)
    sleep(1)
    process.poll()
    if (process.returncode is not None):
        sys.exit(1)
    process.command = command
    _processes.append(process)
    sys.stdout.write('done\n')


# stops a registered process by its name
def stop_process(command):
    for process in _processes:
        processCommand = process.command.split(None, 1)[0]
        if command == processCommand:
            sys.stdout.write('stopping ' + command + '... ')
            sys.stdout.flush()
            process.kill()
            process.wait()
            sys.stdout.write('done\n')


# stops all registered processes
def stop_processes():
    for process in _processes:
        sys.stdout.write('stopping ' + process.command.split(None, 1)[0]
                        + '... ')
        sys.stdout.flush()
        process.terminate()
        process.wait()
        sys.stdout.write('done\n')


# loads a HAL configuraton file
def load_hal_file(filename):
    sys.stdout.write("loading " + filename + '... ')
    sys.stdout.flush()
    subprocess.check_call('halcmd -f ' + filename, shell=True)
    sys.stdout.write('done\n')


# loads a BBIO configuration file
def load_bbio_file(filename):
    check_command('config-pin')
    sys.stdout.write("loading " + filename + '... ')
    sys.stdout.flush()
    subprocess.check_call('config-pin -f ' + filename, shell=True)
    sys.stdout.write('done\n')


# installs a comp RT component
def install_comp(filename):
    install = True
    base = os.path.splitext(os.path.basename(filename))[0]
    flavor = compat.default_flavor()
    modulePath = compat.get_rtapi_config("RTLIB_DIR") + '/' + flavor.name + '/' + base + flavor.mod_ext
    if os.path.exists(modulePath):
        compTime = os.path.getmtime(filename)
        moduleTime = os.path.getmtime(modulePath)
        if (compTime < moduleTime):
            install = False

    if install is True:
        sys.stdout.write("installing " + filename + '... ')
        sys.stdout.flush()
        subprocess.check_call('comp --install ' + filename, shell=True)
        sys.stdout.write('done\n')


# starts realtime
def start_realtime():
    sys.stdout.write("starting realtime...")
    sys.stdout.flush()
    subprocess.check_call('realtime start', shell=True)
    sys.stdout.write('done\n')


# stops realtime
def stop_realtime():
    sys.stdout.write("stopping realtime... ")
    sys.stdout.flush()
    subprocess.check_call('realtime stop', shell=True)
    sys.stdout.write('done\n')


# rip the Machinekit environment
def rip_environment(path=None, force=False):
    if force == False and os.getenv('EMC2_PATH') is not None: # check if already ripped
        return

    if path is None:
        command = None
        scriptFilePath = os.environ['HOME'] + '/.bashrc'
        if os.path.exists(scriptFilePath):
            with open(scriptFilePath) as f:    # use the bashrc
                content = f.readlines()
                for line in content:
                    if 'rip-environment' in line:
                        line = line.strip()
                        if (line[0] == '.'):
                            command = line

        scriptFilePath = os.environ['HOME'] + '/machinekit/scripts/rip-environment'
        if os.path.exists(scriptFilePath):
            command = '. ' + scriptFilePath

        if (command is None):
            sys.stderr.write('Unable to rip environment')
            sys.exit(1)
    else:
        command = '. ' + path + '/scripts/rip-environment'

    process = subprocess.Popen(command + ' && env',
                        stdout=subprocess.PIPE,
                        shell=True)
    for line in process.stdout:
        (key, _, value) = line.partition('=')
        os.environ[key] = value.rstrip()

    sys.path.append(os.environ['PYTHONPATH'])


# checks the running processes and exits when exited
def check_processes():
    for process in _processes:
        process.poll()
        if (process.returncode is not None):
            _processes.remove(process)
            end_session()
            if (process.returncode != 0):
                sys.exit(1)
            else:
                sys.exit(0)


# register exit signal handlers
def register_exit_handler():
    signal.signal(signal.SIGINT, _exitHandler)
    signal.signal(signal.SIGTERM, _exitHandler)


def _exitHandler(signum, frame):
    end_session()
    sys.exit(0)


# set the Machinekit debug level
def set_debug_level(level):
    os.environ['DEBUG'] = str(level)


# set the Machinekit ini
def set_machinekit_ini(ini):
    os.environ['MACHINEKIT_INI'] = ini
