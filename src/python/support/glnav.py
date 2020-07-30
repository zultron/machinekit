from minigl import *
import math
import array, itertools
import numpy as np

def use_pango_font(font, start, count, will_call_prepost=False):
    import pango, cairo, pangocairo
    fontDesc = pango.FontDescription(font)
    a = array.array('b', itertools.repeat(0, 256*256))
    surface = cairo.ImageSurface.create_for_data(a, cairo.FORMAT_A8, 256, 256)
    context = pangocairo.CairoContext(cairo.Context(surface))
    layout = context.create_layout()
    fontmap = pangocairo.cairo_font_map_get_default()
    font = fontmap.load_font(fontmap.create_context(), fontDesc)
    layout.set_font_description(fontDesc)
    metrics = font.get_metrics()
    descent = metrics.get_descent()
    d = pango.PIXELS(descent)
    linespace = metrics.get_ascent() + metrics.get_descent()
    width = metrics.get_approximate_char_width()

    glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT)
    glPixelStorei(GL_UNPACK_SWAP_BYTES, 0)
    glPixelStorei(GL_UNPACK_LSB_FIRST, 1)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 256)
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 256)
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0)
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0)
    glPixelStorei(GL_UNPACK_SKIP_IMAGES, 0)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1)
    glPixelZoom(1, -1)

    base = glGenLists(count)
    for i in range(count):
        ch = unichr(start+i)
        layout.set_text(ch)
        w, h = layout.get_size()
        context.save()
        context.new_path()
        context.rectangle(0, 0, 256, 256)
        context.set_source_rgba(0., 0., 0., 0.)
        context.set_operator (cairo.OPERATOR_SOURCE);
        context.paint()
        context.restore()

        context.save()
        context.set_source_rgba(1., 1., 1., 1.)
        context.set_operator (cairo.OPERATOR_SOURCE);
        context.move_to(0, 0)
        context.update_layout(layout)
        context.show_layout(layout)
        context.restore()

        w, h = pango.PIXELS(w), pango.PIXELS(h)
        glNewList(base+i, GL_COMPILE)
        glBitmap(0, 0, 0, 0, 0, h-d, '');
        if not will_call_prepost: pango_font_pre()
        if w and h: glDrawPixels(w, h, GL_LUMINANCE, GL_UNSIGNED_BYTE, a)
        glBitmap(0, 0, 0, 0, w, -h+d, '');
        if not will_call_prepost: pango_font_post()
        glEndList()

    glPopClientAttrib()
    return base, pango.PIXELS(width), pango.PIXELS(linespace)

def pango_font_pre(rgba=(1., 1., 0., 1.)):
    glPushAttrib(GL_COLOR_BUFFER_BIT)
    glEnable(GL_BLEND)
    glBlendFunc(GL_ONE, GL_ONE)

def pango_font_post():
    glPopAttrib()


def sub(x, y):
    return map(lambda a, b: a-b, x, y)

def dot(x, y):
    t = 0
    for i in range(len(x)):
        t = t + x[i]*y[i]
    return t

def glDistFromLine(x, p1, p2):
    f = map(lambda x, y: x-y, p2, p1)
    g = map(lambda x, y: x-y, x, p1)
    return dot(g, g) - dot(f, g)**2/dot(f, f)

def v3distsq(a,b):
    d = ( a[0] - b[0], a[1] - b[1], a[2] - b[2] )
    return d[0]*d[0] + d[1]*d[1] + d[2]*d[2]

def snaps_to_increment(incr, tol):
    def snap(raw_val):
        m = (raw_val - incr/2) % incr - incr/2
        if abs(m) < tol:
            return raw_val - m
        else:
            return raw_val
    return snap


class CallOnExitWrapper(object):
    def __init__(self, fcall):
        self._counter = 0
        self.fcall = fcall
    def __enter__(self):
        self._counter += 1
    def __exit__(self, type, value, traceback):
        self._counter -= 1
        if self._counter <= 0 and self.fcall is not None:
            self.fcall()
            # Recover from underflow
            self._counter = 0


def with_redraw(f):
    def inner(self, *args, **kw):
        with self.need_redraw:
            return f(self, *args, **kw)
    return inner


class GlUserView:
    """
    Encapsulates basic operations to move the user's viewpoint of a scene safetly
    (i.e. without disturbing any other camera transforms that might occur, due like view animations)
    """
    view_rotation_vectors = (
        (1., 0., 0.), # Latitude
        (0., 0., 1.), # Longitude
        (0., 1., 0.), # Y spin
    )
    empty_world_tf = (
        (0.,)*3,
        (0.,)*3,
        (0.,)*3
    )

    def __init__(self):
        self._center_pt = np.array((0.,) * 3)
        self._lon = 0
        self._lat = 0
        self._spin = 0

        self.need_redraw = CallOnExitWrapper(self._redraw)

        self.last_world_transform = GlUserView.empty_world_tf
        self._view_follows_rotation = True # Controls whether to apply rotation part of world transform
        self._view_follows_translation = False # View follows tool in translation

    @property
    def center_pt(self):
        return self._center_pt

    @property
    def longitude(self):
        return self._lon

    @property
    def spin(self):
        return self._spin

    @property
    def latitude(self):
        return self._lat

    def reset_view(self, center=None, clear_world=False):
        """
        Reset model view rotation matrix and view settings.
        """

        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        if center is not None:
            glTranslatef(-center[0], -center[1], -center[2])

        if not clear_world:
            self.swap_world_view_transform(self.last_world_transform, ignore_previous=True)
        self._lon = 0
        self._lat = 0
        self._spin = 0

    @with_redraw
    def glTranslateUserView(self, view_dx, view_dy, scale=1.0):
        glMatrixMode(GL_MODELVIEW)
        # Unapply user view transform
        mat = glGetDoublev(GL_MODELVIEW_MATRIX)
        glLoadIdentity()
        # Apply local view translation
        glTranslatef(scale * view_dx, scale * view_dy, 0.0)

        # Re-apply user view transform
        glMultMatrixd(mat)

    @with_redraw
    def glRotateUserViewPolar(self, center_pt=None, longitude=None, latitude=None, spin=None, bypass_world=False):
        """
        Rotates the user view to the specified lat / lon coordinates, given
        the center point in model space (uses current centerpoint by default).
        """
        glMatrixMode(GL_MODELVIEW)

        center_pt = np.array(self.center_pt if center_pt is None else center_pt)
        if longitude is None:
            longitude = self.longitude
        if latitude is None:
            latitude = self.latitude
        if spin is None:
            spin = self.spin

        # If starting from identity transform (e.g. when preparing top / side / front view),
        # don't remove the world transform (since it's not currently applied)
        world_view_tf = self.last_world_transform if bypass_world else self.swap_world_view_transform(GlUserView.empty_world_tf)

        # 1) Translate the scene so the view center is at the origin
        glTranslatef(*center_pt)

        # 2) Unrotate our current view rotation (note the reverse order)
        # then rotate to apply the requested user view rotation
        glRotatef(-self.spin, *self.view_rotation_vectors[2])
        glRotatef(-self.longitude, *self.view_rotation_vectors[1])
        glRotatef(latitude - self.latitude, *self.view_rotation_vectors[0])
        glRotatef(longitude, *self.view_rotation_vectors[1])
        glRotatef(spin, *self.view_rotation_vectors[2])

        # 3) Restore the scene's translation back to the original location
        # Note that this function should produce no net translation
        glTranslatef(*(-1. * center_pt))

        # Restore the world transform
        self.swap_world_view_transform(world_view_tf)

        # Finally, update the scene values
        self._lon = longitude
        self._lat = latitude
        self._spin = spin

    def _redraw(self):
        raise NotImplementedError

    def swap_world_view_transform(self, world_tf, ignore_previous=False, follow_translation=False, follow_rotation=True):
        r2d = 180./np.pi
        old_tf = self.last_world_transform
        if self.last_world_transform is not None and not ignore_previous:
            pos, rot, rot_cen = self.last_world_transform
            if self._view_follows_rotation:
                glTranslatef(*(np.array(rot_cen)))
                glRotatef(-rot[0] * r2d, 1, 0, 0)
                glRotatef(-rot[1] * r2d, 0, 1, 0)
                glRotatef(-rot[2] * r2d, 0, 0, 1)
                glTranslatef(*(np.array(rot_cen) * -1.))
            if self._view_follows_translation:
                glTranslatef(*(np.array(pos) * -1.))

        # Get world transform in standard form
        self.last_world_transform = world_tf
        pos_new, rot_new, rot_cen_new = world_tf
        if follow_translation:
            glTranslatef(*pos_new)
        if follow_rotation:
            glTranslatef(*rot_cen_new)
            glRotatef(rot_new[2] * r2d, 0, 0, 1)
            glRotatef(rot_new[1] * r2d, 0, 1, 0)
            glRotatef(rot_new[0] * r2d, 1, 0, 0)
            glTranslatef(*(np.array(rot_cen_new)*-1))
        self._view_follows_translation = follow_translation
        self._view_follows_rotation = follow_rotation

        return old_tf


class GlNavBase(GlUserView):

    def __init__(self):
        GlUserView.__init__(self)
        self.canon = None
        # Current coordinates of the mouse.
        self.xmouse = 0
        self.ymouse = 0

        # The _back color
        self.r_back = 1.
        self.g_back = 0.
        self.b_back = 1.

        # In ortho mode, "distance" is the scene width.
        # In perspective mode, it's the distance between near and far planes
        self.distance = 10.0
        # TODO use separate zoom controls for each view mode since perspective now gets its own
        #self.ortho_view_width = 10.0
        self.zoom_ratio_default = 1.2

        # Field of view in y direction
        self.fovy = 30.0

        # Position of clipping planes.
        self.near = 0.1
        self.far = 1000.0

        # View settings
        self.perspective_view = False
        self.use_perspective_proj = False
        self.minlat = -180
        self.maxlat = 180

        self.y0 = 0
        self.original_zoom = 0

        # Zoom out by this factor to see the extent labels for a loaded model.
        # At 1.0, the preview toolpath just fills the screen
        self.ortho_view_scale = 1.75

    # Abstract methods that must be implemented
    def _redraw(self):
        raise NotImplementedError

    def extents_info(self):
        raise NotImplementedError

    def is_lathe(self):
        raise NotImplementedError

    def is_rear_tool_post_lathe(self):
        raise NotImplementedError

    def activate(self):
        raise NotImplementedError

    def winfo_width(self):
        raise NotImplementedError

    def winfo_height(self):
        raise NotImplementedError

    # This should almost certainly be part of some derived class.
    # But I have put it here for convenience.
    def basic_lighting(self):
        """\
        Set up some basic lighting (single infinite light source).

        Also switch on the depth buffer."""

        self.activate()
        glLightfv(GL_LIGHT0, GL_POSITION, (1, -1, 1, 0))
        glLightfv(GL_LIGHT0, GL_AMBIENT, (.4, .4, .4, 1))
        glLightfv(GL_LIGHT0, GL_DIFFUSE, (.6, .6, .6, 1))
        glEnable(GL_LIGHTING)
        glEnable(GL_LIGHT0)
        glDepthFunc(GL_LESS)
        glEnable(GL_DEPTH_TEST)
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()

    @with_redraw
    def set_background(self, r, g, b):
        """Change the background colour of the widget."""

        self.r_back = r
        self.g_back = g
        self.b_back = b

    @with_redraw
    def set_centerpoint(self, x, y, z):
        """
        Set the new center point for the model.
        This is where we are looking.
        NOTE: to keep the view consistent, it's important to remove / reapply
        the scene rotation. Do not directly change the centerpoint values here.
        """

        # FIXME need to track the old centerpoint until the rotation is actually used
        self._center_pt = np.array((x, y, z))

    @with_redraw
    def set_latitudelimits(self, minlat, maxlat):
        """Set the new "latitude" limits for rotations."""
        if maxlat > 180:
            return
        if minlat < -180:
            return
        if maxlat <= minlat:
            return
        self.maxlat = maxlat
        self.minlat = minlat

    @with_redraw
    def set_eyepoint(self, distance):
        """Set how far the eye is from the position we are looking."""
        self.distance = distance

    @with_redraw
    def set_eyepoint_from_extents(self, scene_width, scene_height=None, view_scale=None):
        """Set how far the eye is from the position we are looking
        based on the screen width and height of a subject."""
        w = self.winfo_width()
        h = self.winfo_height()
        view_scale = self.ortho_view_scale if view_scale is None else view_scale

        scaled_height = scene_height * w / h if scene_height is not None else scene_width
        effective_width_sq = (max(2.0, scene_width, scaled_height) * view_scale) ** 2.
        self.set_eyepoint(effective_width_sq)
        #print "w, h = {}, {}, scene size: {}, {}, scaled_height = {}".format(w, h, scene_width, scene_height, scaled_height)

    @with_redraw
    def reset(self):
        """Reset rotation matrix for this widget."""

        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()


    def recordMouse(self, x, y):
        self.xmouse = x
        self.ymouse = y

    def startRotate(self, x, y):
        self.recordMouse(x, y)

    @with_redraw
    def scale_scene(self, x, y):
        """Scale the scene.  Achieved by moving the eye position.

        Dragging up zooms in, while dragging down zooms out
        """
        scale = 1 - 0.01 * (y - self.ymouse)
        # do some sanity checks, scale no more than
        # 1:1000 on any given click+drag
        if scale < 0.001:
            scale = 0.001
        elif scale > 1000:
            scale = 1000
        newdistance = self.distance * scale
        if newdistance < 1e-30 or newdistance > 1e30:
            return
        self.distance = newdistance
        self.recordMouse(x, y)

    @with_redraw
    def rotate(self, x, y):
        """Perform rotation of scene."""

        self.activate()
        self.restore_projection()
        self.glRotateScene(0.5, x - self.xmouse, y - self.ymouse)
        self.recordMouse(x, y)

    @with_redraw
    def translate(self, x, y):
        """Perform translation of scene."""

        self.activate()

        # Scale mouse translations to object viewplane so object tracks with mouse
        view_scale = self.find_mouse_scale_from_view()

        view_dx = view_scale * (x - self.xmouse)
        view_dy = -view_scale * (y - self.ymouse)
        self.glTranslateUserView(view_dx, view_dy)
        self.recordMouse(x, y)

    def find_mouse_scale_from_view(self):
        win_height = max(1, self.winfo_height())
        win = gluProject(*self.center_pt)
        obj = gluUnProject(win[0], win[1] + 0.5 * win_height, win[2])
        dist = math.sqrt(v3distsq(obj, self.center_pt))
        scale = abs(dist / (0.5 * win_height))
        return scale

    @with_redraw
    def glRotateScene(self, scale, mouse_dx, mouse_dy):
        snap = snaps_to_increment(90, 2)

        lat_new = min(self.maxlat, max(self.minlat, self.latitude + mouse_dy * scale))
        lon_new = (self.longitude + mouse_dx * scale) % 360

        self.glRotateUserViewPolar(longitude=snap(lon_new), latitude=snap(lat_new))

    @with_redraw
    def set_viewangle(self, lat, lon):
        self.glRotateUserViewPolar(latitude=lat, longitude=lon)

    def get_zoom_distance(self):
        data = self.distance
        return data

    @with_redraw
    def set_zoom_distance(self,data):
        self.distance = data

    @with_redraw
    def zoom_in(self, ratio=None):
        self.distance /= (ratio or self.zoom_ratio_default)
        #print "distance =", self.distance

    @with_redraw
    def zoom_out(self, ratio=None):
        self.distance *= (ratio or self.zoom_ratio_default)
        #print "distance =", self.distance

    def startZoom(self, y):
        self.y0 = y
        self.original_zoom = self.distance

    @with_redraw
    def continueZoom(self, y):
        dy = y - self.y0
        self.distance = self.original_zoom * pow(1.25, dy / 16.)

    def getRotateMode(self): return False

    def translateOrRotate(self, x, y):
        if self.getRotateMode():
            self.rotate(x, y)
        else:
            self.translate(x, y)

    def rotateOrTranslate(self, x, y):
        if not self.getRotateMode():
            self.rotate(x, y)
        else:
            self.translate(x, y)

    @with_redraw
    def set_projection_mode(self, use_perspective):
        self.use_perspective_proj = use_perspective

    @with_redraw
    def disable_projection(self):
        """
        Unconditionally disable perspective projection (for specific orthographic views)
        """
        self.perspective_view = False

    @with_redraw
    def restore_projection(self):
        """
        Restore the currently active projection mode for general viewing (can be orthographic or perspective)
        """
        self.perspective_view = self.use_perspective_proj

    def set_ortho_view_scale(self, vs):
        self.ortho_view_scale = max(vs, 1.0)

    # NOTE: set_view_XXX functions follow a standard pattern
    # * reset the GL Model View matrix and the view center / rotation settings
    # * get the current view region of interest (machine bounds or extents of loaded program)
    # * set up eyepoints
    # * Specify perpective or orthographic view ("iso" view is actually perspective projection, not true isometric)
    # * Translate view to be centered on the region of interest
    # * Rotate view to the desired orientation about the center point

    @with_redraw
    def set_view_x(self):
        mid, size, scale = self.extents_info()
        self.set_eyepoint_from_extents(size[1], size[2], scale)
        self.disable_projection()

        self.reset_view(center=mid)
        self.glRotateUserViewPolar(mid, longitude=270, latitude=-90)

    @with_redraw
    def set_view_y(self):
        mid, size, scale = self.extents_info()
        if self.is_lathe():
            width = size[2]
            height = size[0]
        else:
            width = size[0]
            height = size[2]
        self.set_eyepoint_from_extents(width, height, scale)
        self.disable_projection()

        center_pt = np.array(mid)
        center_pt[0] *= -1. if self.is_rear_tool_post_lathe() else 1.
        self.reset_view(center=center_pt)
        if self.is_lathe():
            spin = 90
        else:
            spin = 0
        self.glRotateUserViewPolar(center_pt, longitude=0, latitude=-90, spin=spin)

    @with_redraw
    def set_view_y2(self):
        """
        lathe backtool display
        """
        mid, size, scale = self.extents_info()
        self.set_eyepoint_from_extents(size[0], size[2], scale)
        self.disable_projection()

        self.reset_view(center=mid)
        if self.is_lathe():
            spin = 90 if self.is_rear_tool_post_lathe() else -90
        else:
            spin = 0
        self.glRotateUserViewPolar(mid, longitude=0, latitude=90, spin=spin)

    @with_redraw
    def set_view_z(self):
        mid, size, scale = self.extents_info()
        self.set_eyepoint_from_extents(size[0], size[1], scale)
        self.disable_projection()

        self.reset_view(center=mid)
        self.glRotateUserViewPolar(mid, longitude=0, latitude=0)

    @with_redraw
    def set_view_z2(self):
        mid, size, scale = self.extents_info()
        self.set_eyepoint_from_extents(size[1], size[0], scale)
        self.disable_projection()

        self.reset_view(center=mid)
        self.glRotateUserViewPolar(mid, longitude=270, latitude=0)


    @with_redraw
    def reset_iso_view_zoom(self):
        mid, sizes, scale = self.extents_info()
        size = (sizes[0] ** 2 + sizes[1] ** 2 + sizes[2] ** 2) ** .5
        if size > 1e99:
            size = 5. # in case there are no moves in the preview

        if self.perspective_view:
            w = self.winfo_width()
            h = self.winfo_height()
            fovx = self.fovy * w / h
            fov = min(fovx, self.fovy)
            self.set_eyepoint((size * 1.1 + 1.0) / 2 / math.sin(fov * math.pi / 180 / 2))
        else:
            self.set_eyepoint_from_extents(size, view_scale=scale)
        return mid


    @with_redraw
    def set_view_p(self):
        self.restore_projection()
        mid = self.reset_iso_view_zoom()

        self.reset_view(center=mid)
        self.glRotateUserViewPolar(mid, longitude=-25, latitude=-60)

# vim:ts=8:sts=4:sw=4:et:
