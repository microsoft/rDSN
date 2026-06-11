from WebStudioLib import *
from WebStudioUtil import *

class StaticFileHandler(webapp2.RequestHandler):
    path = ''
    def get(self, path):
        base_path = os.path.realpath(self.path)
        abs_path = os.path.realpath(os.path.join(base_path, path))
        if abs_path != base_path and not abs_path.startswith(base_path + os.sep):
            self.response.set_status(403)
            return
        if os.path.isdir(abs_path)  != 0:
            self.response.set_status(403)
            return
        try:
            with open(abs_path, 'rb') as f:
                content_type = mimetypes.guess_type(abs_path)[0] or 'application/octet-stream'
                self.response.headers.add_header('Content-Type', content_type)
                self.response.out.write(f.read())
        except IOError:
            self.response.set_status(404)

class AppStaticFileHandler(StaticFileHandler):
    def __init__(self, request, response):
        # Set self.request, self.response and self.app.
        self.initialize(request, response)
        self.path = './'

class LocalStaticFileHandler(StaticFileHandler):
    def __init__(self, request, response):
        # Set self.request, self.response and self.app.
        self.initialize(request, response)
        self.path = GetWebStudioDirPath() + '/local/'
