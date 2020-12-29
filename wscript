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
    opt.load('msvc')


def configure(conf):
    # Prefers msvc, but could also use conf.load('clang++') instead
    conf.load('msvc')
    #conf.find_program('riscv64-unknown-elf-c++', name='gccriscv', exts='.exe')
    #conf.find_program('riscv64-unknown-elf-ld', name='ldriscv', exts='.exe')
    #conf.find_program('riscv64-unknown-elf-objdump', name='objdumpriscv', exts='.exe')
    #conf.find_program('riscv64-unknown-elf-readelf', name='readelfriscv', exts='.exe')

def build(bld):

    platform_defines = ['_CRT_SECURE_NO_WARNINGS', 'CAT_WINDOWS']
    slib = '%ProgramFiles%Windows Kits/10/Lib/'
    sinc = '%ProgramFiles%Windows Kits/10/Include/'
    win_sdk_lib_path = os.path.expandvars(slib+'10.0.19041.0/um/x64/')
    winsdkinclude = os.path.expandvars(sinc+'10.0.19041.0/um/x64/')
    wsdkincludeshared = os.path.expandvars(sinc+'10.0.19041.0/shared')
    includes = ['source', 'includes', 'buildtools',
                winsdkinclude, wsdkincludeshared]
    libs = ['user32', 'Comdlg32', 'gdi32', 'ole32',
            'kernel32', 'winmm', 'ws2_32']

    # RELEASE
    compile_flags = ['/permissive-', '/std:c++17', '/arch:AVX',
                        '/GL', '/WX', '/Ox', '/Ot', '/Oy', '/fp:fast',
                        '/Qfast_transcendentals', '/Zi', '/EHsc',
                        '/FS', '/D_SECURE_SCL 0',
                        '/D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS']
    linker_flags = [ '/LTCG', '/RELEASE']

    # DEBUG
    #compile_flags = ['/permissive-', '/std:c++17', '/arch:AVX',
    #                 '/GL', '/WX', '/Od', '/DDEBUG', '/fp:fast',
    #                 '/Qfast_transcendentals', '/Zi', '/Gs',
    #                 '/EHsc', '/FS']
    #linker_flags = ['/DEBUG']

    bld.program(
        source=glob.glob('*.cpp'),
        cxxflags=compile_flags,
        ldflags=linker_flags,
        target='riscvtool',
        defines=platform_defines,
        includes=includes,
        libpath=[win_sdk_lib_path],
        lib=libs)
