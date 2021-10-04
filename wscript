import glob
import os
import platform
from waflib.TaskGen import extension, feature, task_gen
from waflib.Task import Task
from waflib import Build

VERSION = '0.1'
APPNAME = 'riscvtool'

top = '.'


def options(opt):
    # Prefers msvc, but could also use conf.load('clang++') instead
    if ('.exe' in os.environ['COMSPEC']):
        opt.load('msvc')
    else:
        opt.load('clang++')

def configure(conf):
    # Prefers msvc, but could also use conf.load('clang++') instead
    if ('.exe' in os.environ['COMSPEC']):
        conf.load('msvc')
    else:
        conf.load('clang++')

def build(bld):

    if ('.exe' in os.environ['COMSPEC']):
        platform_defines = ['_CRT_SECURE_NO_WARNINGS', 'CAT_WINDOWS']
    else:
        platform_defines = ['_CRT_SECURE_NO_WARNINGS', 'CAT_LINUX']
    includes = ['source', 'includes']

    # RELEASE
    sdk_lib_path = []
    libs = []
    compile_flags = []
    linker_flags = []

    # Build risctool
    bld.program(
        source=glob.glob('*.cpp'),
        cxxflags=compile_flags,
        ldflags=linker_flags,
        target='riscvtool',
        defines=platform_defines,
        includes=includes,
        libpath=[sdk_lib_path],
        lib=libs)
