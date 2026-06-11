from WebStudioLib import *
from WebStudioUtil import *
from WebStudioBase import *
from WebStudioApi import *

def SafeJoin(base_path, *paths):
    base_path = os.path.realpath(base_path)
    target_path = os.path.realpath(os.path.join(base_path, *paths))
    if target_path != base_path and not target_path.startswith(base_path + os.sep):
        raise ValueError("invalid path")
    return target_path

class PageMainHandler(BaseHandler):
    def get(self):
        self.render_template('main.html')

class PageTableHandler(BaseHandler):
    def get(self):
        self.render_template_Vue('table.html')

class PageTaskAnalyzerHandler(BaseHandler):
    def get(self):
        self.render_template_Vue('task_analyzer.html')

class PageQueueHandler(BaseHandler):
    def get(self):
        self.render_template_Vue('queue.html')

class PageCliHandler(BaseHandler):
    def get(self):
        self.render_template('cli.html')

class PageBashHandler(BaseHandler):
    def get(self):
        self.render_template('bash.html')

class PageEditorHandler(BaseHandler):
    def get(self):
        params = {}
        dir = os.getcwd()
        working_dir = self.request.get('working_dir')
        file_name = self.request.get('file_name')
        if file_name != '':
            read_file = open(os.path.join(dir,working_dir, file_name),'r')
            content = read_file.read()
            read_file.close()
        else:
            content = ''

        dir_list = []
        lastPath = ''
        for d in working_dir.split('/'):
            if lastPath!='':
                lastPath += '/'
            lastPath +=d
            dir_list.append({'path':lastPath,'name':d})
        params['FILES'] = [f for f in os.listdir(os.path.join(dir,working_dir)) if os.path.isfile(os.path.join(dir,working_dir,f))]
        params['FILEFOLDERS'] = [f for f in os.listdir(os.path.join(dir,working_dir)) if os.path.isdir(os.path.join(dir,working_dir,f))]
        params['WORKING_DIR'] = working_dir
        params['DIR_LIST'] = dir_list
        params['CONTENT'] = content 
        params['FILE_NAME'] = file_name 
        
        self.render_template('editor.html',params)
    def post(self):
        content = self.request.get('content')
        dir = os.path.dirname(__file__)
        working_dir = self.request.get('working_dir')
        file_name = self.request.get('file_name')
        if file_name != '':
            write_file = open(os.path.join(dir,working_dir, file_name),'w')
            write_file.write(content)
            write_file.close()
            self.response.write("Successfully saved!")
        else:
            self.response.write("No file opened!")

class PageConfigureHandler(BaseHandler):
    def get(self):
        self.render_template_Vue('configure.html')

class PageFileViewHandler(BaseHandler):
    def get(self):
        params = {}
        working_dir = self.request.get('working_dir')
        root_dir = self.request.get('root_dir')
        
        if root_dir == 'local':
            dir = os.path.dirname(GetWebStudioDirPath()+'/local/')
        elif root_dir == 'app':
            dir = os.path.dirname(os.getcwd()+"/")
        elif root_dir == '':
            root_dir = 'app'
            dir = os.path.dirname(os.getcwd()+"/")
        

        try:
            params['FILES'] = [f for f in os.listdir(os.path.join(dir,working_dir)) if os.path.isfile(os.path.join(dir,working_dir,f))]
            params['FILEFOLDERS'] = [f for f in os.listdir(os.path.join(dir,working_dir)) if os.path.isdir(os.path.join(dir,working_dir,f))]
        except:
            self.response.write('Cannot find the specified file path, please check again')
            return

        dir_list = []
        lastPath = ''
        for d in working_dir.split('/'):
            if lastPath!='':
                lastPath += '/'
            lastPath +=d
            dir_list.append({'path':lastPath,'name':d})
        params['WORKING_DIR'] = working_dir
        params['ROOT_DIR'] = root_dir
        params['DIR_LIST'] = dir_list
        
        self.render_template('fileview.html',params)
    def post(self):
        params = {}
        dir = os.path.dirname(os.getcwd()+"/")
        working_dir = self.request.get('working_dir')
        
        try:
            raw_file = self.request.get('fileToUpload')
            file_name = self.request.get('file_name')
            savedFile = open(os.path.join(dir,working_dir,file_name),'wb')
            savedFile.write(raw_file)
            savedFile.close()

            params['RESPONSE'] = 'success'
        except:
            params['RESPONSE'] = 'fail'

        dir_list = []
        lastPath = ''
        for d in working_dir.split('/'):
            if lastPath!='':
                lastPath += '/'
            lastPath +=d
            dir_list.append({'path':lastPath,'name':d})
        params['FILES'] = [f for f in os.listdir(os.path.join(dir,working_dir)) if os.path.isfile(os.path.join(dir,working_dir,f))]
        params['FILEFOLDERS'] = [f for f in os.listdir(os.path.join(dir,working_dir)) if os.path.isdir(os.path.join(dir,working_dir,f))]
        params['WORKING_DIR'] = working_dir
        params['DIR_LIST'] = dir_list

        self.render_template('fileview.html',params)

class PageAnalyzerHandler(BaseHandler):
    def get(self):
        self.render_template_Vue('analyzer.html')

class PageCounterViewHandler(BaseHandler):
    def get(self):
        self.render_template_Vue('counterview.html')
        
class PageStoreHandler(BaseHandler):
    def get(self):
        self.render_template_Vue('store.html')
    def post(self):        
        raw_file = self.request.get('fileToUpload')
        file_path = self.request.get('file_path')
        
        raw_icon = self.request.get('iconToUpload')
        icon_path = self.request.get('icon_path');
        
        pack_name = self.request.get('file_name');        
        author = self.request.get('author')
        description = self.request.get('description')
        schema_info = self.request.get('schema_info')
        schema_type = self.request.get('schema_type')
        rpc_type = self.request.get('rpc_type')
        parameters = self.request.get('parameters')
        if_stateful = self.request.get('if_stateful')
        
        # 
        packages_dir = os.path.join(GetWebStudioDirPath(),'local','packages')
        pack_dir = SafeJoin(packages_dir, pack_name)
        if not os.path.exists(pack_dir):
            os.makedirs(pack_dir)

        # save uploaded package 
        savedFile = open(SafeJoin(packages_dir, file_path), 'wb')
        savedFile.write(raw_file)
        savedFile.close()

        # save icon file 
        iconFile = open(SafeJoin(pack_dir, icon_path), 'wb')
        iconFile.write(raw_icon)
        iconFile.close()
        
        # save to db
        sqlOp(op='save',dataType='app_package',val_list=[
            pack_name,            
            author,
            description,
            schema_info,
            schema_type,
            rpc_type,
            parameters,
            if_stateful,
            file_path,
            icon_path
        ])

        return webapp2.redirect('/store.html')

class PageMulticmdHandler(BaseHandler):
    def get(self):
        self.render_template_Vue('multicmd.html')

class PageServiceMetaHandler(BaseHandler):
    def get(self):
        self.render_template_Vue('service_meta.html')

class PageMachineHandler(BaseHandler):
    def get(self):
        self.render_template_Vue('machine.html')

class PageSettingHandler(BaseHandler):
    def get(self):
        self.render_template_Vue('setting.html')
