from minigl import *
from contextlib import contextmanager


@contextmanager
def gl_new_list(list_obj, list_mode=GL_COMPILE):
    try:
        glNewList(list_obj, list_mode)
        yield
    finally:
        glEndList()


@contextmanager
def gl_line_width(w):
    glLineWidth(w)
    try:
        yield
    finally:
        glLineWidth(1)


@contextmanager
def gl_begin(mode):
    glBegin(mode)
    try:
        yield
    finally:
        glEnd()


@contextmanager
def gl_enable(mode):
    """
    Enables the specified mode, and disables it on context exit
    NOTE: this ignores the current enable / disable state
    """
    glEnable(mode)
    try:
        yield
    finally:
        glDisable(mode)


@contextmanager
def gl_disable(mode):
    """
    Disables the specified mode, and enables it on context exit
    NOTE: this ignores the current enable / disable state
    """
    glDisable(mode)
    try:
        yield
    finally:
        glEnable(mode)


@contextmanager
def gl_push_matrix():
    glPushMatrix()
    try:
        yield
    finally:
        glPopMatrix()

