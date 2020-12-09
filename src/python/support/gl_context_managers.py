import minigl
from contextlib import contextmanager


@contextmanager
def gl_gen_new_list(list_mode=minigl.GL_COMPILE):
    """
    Context manager to do the job of glGenLists, glNewList in a single step.
    This ensures that glNewList and glEndList is only called if the list_id created is valid.
    If it is valid, both functions are always called regardless of exceptions.
    :param list_mode: Enum to pass to glNewList that specifies how much processing to do
     (either just compile or compile and draw)

    Usage example:
        with gl_gen_new_list(GL_COMPILE) as my_list_id:
            # gl draw calls here
        return my_list_id
    """
    list_id = minigl.glGenLists(1)
    try:
        if list_id:
            minigl.glNewList(list_id, list_mode)
        yield list_id
    finally:
        if list_id:
            minigl.glEndList()


@contextmanager
def gl_new_list(list_id, list_mode=minigl.GL_COMPILE):
    """
    Context manager to replace a pair of glNewList / glEndList calls.
    This ensures that glNewList and glEndList is only called if the list_id created is valid.
    If it is valid, both functions are always called regardless of exceptions.
    :param list_mode: Enum to pass to glNewList that specifies how much processing to do
     (either just compile or compile and draw)

    Usage example:
        my_list_id = glGenLists(1)
        with gl_new_list(my_list_id, GL_COMPILE):
            # gl draw calls here
        return my_list_id
    """
    try:
        if list_id:
            minigl.glNewList(list_id, list_mode)
        yield list_id
    finally:
        if list_id:
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

