import minigl
from contextlib import contextmanager


@contextmanager
def gl_gen_new_list(list_mode=minigl.GL_COMPILE):
    list_id = minigl.glGenLists(1)
    try:
        if list_id:
            minigl.glNewList(list_id, list_mode)
        yield list_id
    finally:
        if list_id:
            minigl.glEndList()


@contextmanager
def gl_new_list(list_obj, list_mode=minigl.GL_COMPILE):
    try:
        if list_obj:
            minigl.glNewList(list_obj, list_mode)
        yield list_obj
    finally:
        if list_obj:
            minigl.glEndList()


@contextmanager
def gl_line_width(w):
    minigl.glLineWidth(w)
    try:
        yield
    finally:
        minigl.glLineWidth(1)


@contextmanager
def gl_begin(mode):
    minigl.glBegin(mode)
    try:
        yield
    finally:
        minigl.glEnd()


@contextmanager
def gl_enable(mode):
    """
    Enables the specified mode, and disables it on context exit
    NOTE: this ignores the current enable / disable state
    """
    minigl.glEnable(mode)
    try:
        yield
    finally:
        minigl.glDisable(mode)


@contextmanager
def gl_disable(mode):
    """
    Disables the specified mode, and enables it on context exit
    NOTE: this ignores the current enable / disable state
    """
    minigl.glDisable(mode)
    try:
        yield
    finally:
        minigl.glEnable(mode)


@contextmanager
def gl_push_matrix():
    minigl.glPushMatrix()
    try:
        yield
    finally:
        minigl.glPopMatrix()

@contextmanager
def gl_quadric():
    q = minigl.gluNewQuadric()
    try:
        yield q
    finally:
        minigl.gluDeleteQuadric(q)

