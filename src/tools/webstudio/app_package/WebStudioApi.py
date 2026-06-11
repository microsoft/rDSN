from WebStudioLib import *
from WebStudioUtil import *
from WebStudioBase import *

sqlDataType = {}
sqlDataType['counter_view'] = {
    'elems': [
        {'name': 'name','type': 'text'},
        {'name': 'author','type': 'text'},
        {'name': 'description','type': 'text'},
        {'name': 'counterList','type': 'text'},
        {'name': 'graphtype','type': 'text'},
        {'name': 'interval','type': 'text'},
    ],
}

sqlDataType['app_package'] = {
    'elems': [
        {'name': 'name','type': 'text'},
        {'name': 'author','type': 'text'},
        {'name': 'description','type': 'text'},
        {'name': 'schema_info','type': 'text'},
        {'name': 'schema_type','type': 'text'},
        {'name': 'rpc_type','type': 'text'}, # 5
        {'name': 'parameters','type': 'text'},
        {'name': 'if_stateful','type': 'text'},
        {'name': 'file_name','type': 'text'},
        {'name': 'icon_name','type': 'text'},
    ],
}

sqlDataType['cmd_scenario'] = {
    'elems': [
        {'name': 'name','type': 'text'},
        {'name': 'author','type': 'text'},
        {'name': 'description','type': 'text'},
        {'name': 'machines','type': 'text'},
        {'name': 'cmdtext','type': 'text'},
        {'name': 'interval','type': 'text'},
        {'name': 'times','type': 'text'},
    ],
}

TCreate = jinja2.Template('CREATE TABLE IF NOT EXISTS {{dataType}} ({% for elem in elems %}{{elem.name}} {{elem.type}}{% if not loop.last %},{% endif %}{% endfor %});')

def sqlOp(op='',dataType='',dataName='',val_list=''):
    res = None
    if dataType not in sqlDataType:
        raise ValueError("Invalid data type: %s" % dataType)

    local_dir = os.path.join(GetWebStudioDirPath(),'local')
    if not os.path.exists(local_dir):
        os.makedirs(local_dir)

    conn = sqlite3.connect(os.path.join(GetWebStudioDirPath(),'local','data.db'))
    c = conn.cursor()
    
    c.execute(TCreate.render({'dataType':dataType,'elems': sqlDataType[dataType]['elems']}))
    if op == 'save':
        placeholders = ','.join(['?' for _ in val_list])
        c.execute("DELETE FROM %s WHERE name = ?" % dataType, (val_list[0],))
        c.execute("INSERT INTO %s VALUES (%s)" % (dataType, placeholders), val_list)
    elif op == 'load':
        c.execute("SELECT * FROM %s" % dataType)
        res = list(c.fetchall())
    elif op == 'delete':
        c.execute("DELETE FROM %s WHERE name = ?" % dataType, (dataName,))
    elif op == 'detail':
        c.execute("SELECT * FROM %s WHERE name = ?" % dataType, (dataName,))
        row = c.fetchone()
        res = list(row) if row else []
        
    conn.commit()
    conn.close()
    return res

class ApiBashHandler(BaseHandler):
    def get(self):
        self.response.set_status(403)
        self.response.write('Bash API is disabled')

class ApiPsutilHandler(BaseHandler):
    def get(self):
        queryRes = {}
        queryRes['cpu'] = psutil.cpu_percent(interval=1);
        queryRes['memory'] = psutil.virtual_memory()[2];
        queryRes['disk'] = psutil.disk_usage('/')[3];
        queryRes['networkio'] = psutil.net_io_counters()
        self.response.write(json.dumps(queryRes))

class ApiSaveViewHandler(BaseHandler):
    def post(self):
        sqlOp(op='save',dataType='counter_view',val_list=[
                self.request.get('name'),
                self.request.get('author'),
                self.request.get('description'),
                self.request.get('counterList'),
                self.request.get('graphtype'),
                self.request.get('interval')
        ])

        self.response.write('view "'+ self.request.get('name') +'" is successfully saved!')

class ApiLoadViewHandler(BaseHandler):
    def post(self): 
        viewList = []
        for view in sqlOp(op='load',dataType='counter_view'):
            viewList.append({'name':view[0],'author':view[1],'description':view[2],'counterList':view[3],'graphtype':view[4],'interval':view[5]})

        self.SendJson(viewList)

class ApiDelViewHandler(BaseHandler):
    def post(self): 
        sqlOp(op='delete',dataType='counter_view',dataName=self.request.get('name'))

        self.response.write('success')

class ApiLoadPackHandler(BaseHandler):
    def post(self):
        pack_dir = os.path.join(GetWebStudioDirPath(),'local','packages')
        if not os.path.exists(pack_dir):
            os.makedirs(pack_dir)

        packList = []
        for pack in sqlOp(op='load',dataType='app_package'):
            packList.append({'name':pack[0],'author':pack[1], 'parameters':pack[6], 'if_stateful':pack[7],'file_name':pack[8],'icon_name':pack[9]})
        
        self.SendJson(packList)    

class ApiPackDetailHandler(BaseHandler):
    def post(self):
        pack_info = sqlOp(op='detail',dataType='app_package',dataName=self.request.get('id'))

        if pack_info == [] :
            self.response.write('cannot find the package : ' + self.request.get('id'))
            return

        ret = {'name' : pack_info[0], \
            'author' : pack_info[1], \
            'description' : pack_info[2], \
            'schema_info' : pack_info[3], \
            'schema_type' : pack_info[4], \
            'rpc_type' : pack_info[5], \
            'parameters' : pack_info[6], \
            'is_stateful' : pack_info[7], \
            'file_name' : pack_info[8], \
            'icon_name' : pack_info[9]};

        self.SendJson(ret)


class ApiDelPackHandler(BaseHandler):
    def post(self):
        packName = self.request.get('name')

        sqlOp(op='delete',dataType='app_package',dataName=packName)

        try:            
            pack_dir = os.path.join(GetWebStudioDirPath(),'local','packages', packName);
            shutil.rmtree(pack_dir)
            print("remove " + pack_dir)
            self.response.write('success')
        except:
            self.response.write('fail')

class ApiSaveScenarioHandler(BaseHandler):
    def post(self):
        sqlOp(op='save',dataType='cmd_scenario',val_list=[
                self.request.get('name'),
                self.request.get('author'),
                self.request.get('description'),
                self.request.get('machines'),
                self.request.get('cmdtext'),
                self.request.get('interval'),
                self.request.get('times')
        ])

        self.response.write('success')

class ApiLoadScenarioHandler(BaseHandler):
    def post(self): 
        scenarioList = []
        for scenario in sqlOp(op='load',dataType='cmd_scenario'):
            scenarioList.append({'name':scenario[0],'author':scenario[1],'description':scenario[2],'machines':scenario[3],'cmdtext':scenario[4],'interval':scenario[5],'times':scenario[6]})

        self.SendJson(scenarioList)
class ApiDelScenarioHandler(BaseHandler):
    def post(self): 
        sqlOp(op='delete',dataType='cmd_scenario',dataName=self.request.get('name'))

        self.response.write('success')
