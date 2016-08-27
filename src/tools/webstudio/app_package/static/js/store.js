document.getElementById("iconToUpload").onchange = function () {
    document.getElementById("iconpath").value = this.value.replace(/^.*[\\\/]/, '');
};

document.getElementById("fileToUpload").onchange = function () 
{
    var packname = this.value;
    var pack = '';
    var suffix1=".zip";
    var suffix2=".tar.gz";
    
    if (packname.indexOf(suffix1, packname.length - suffix1.length) != -1)
    {
        pack = packname.replace(/^.*[\\\/]/, '').slice(0, - suffix1.length);
    }
    else if (packname.indexOf(suffix2, packname.length - suffix2.length) != -1)
    {
        pack = packname.replace(/^.*[\\\/]/, '').slice(0, - suffix2.length);
    }
    else
    {
        alert("invalid file type, only *.zip and *.tar.gz are allowed");
        document.getElementById('fileToUpload').value = '';
        return;
    }
    
    $('#packname').val(pack);
    var flag = true;
    for(index in vm.$data.packList)
    {
        if(vm.$data.packList[index].name == $('#packname').val())
        {
            vm.$set('ifNameDuplicated', true);
            flag = false;
            break;
        }
    }
    if(flag)
        vm.$set('ifNameDuplicated', false);
    
    document.getElementById("filepath").value = this.value.replace(/^.*[\\\/]/, '');    
};

function validateForm() 
{
    
    var fileToUpload = document.forms["fileForm"]["fileToUpload"].value;
    if (fileToUpload == null || fileToUpload == "") {
        alert("You must choose a file to upload");
        return false;
    }
    
    document.forms["fileForm"]["file_name"].value = $('#packname').val();
    document.forms["fileForm"]["author"].value = $('#author').val();
    document.forms["fileForm"]["description"].value = $('#description').val();
    document.forms["fileForm"]["schema_info"].value = $('#schema_info_in').val();
    document.forms["fileForm"]["schema_type"].value = $('#schema_type_in').val();
    document.forms["fileForm"]["rpc_type"].value = $('#rpc_type_in').val();
    document.forms["fileForm"]["is_stateful"].value = $('#is_stateful').val();
    document.forms["fileForm"]["icon_path"].value = $('#iconpath').val();
    document.forms["fileForm"]["file_path"].value = $('#filepath').val();
    
    var params_map = {};
    for(index in vm.$data.paramList)
    {
        var kv = vm.$data.paramList[index];
        params_map[kv.key] = kv.value;
    }
    
    document.forms["fileForm"]["parameters"].value = JSON.stringify(params_map);

    return true;
}

var vm = new Vue({
    el: '#app',
    data:{
        packList: [],

        detail_pack : null,

        app_name: '',
        partition_count: 0,
        replica_count: 0,
        success_if_exist: true,
        deploy_pack : null,
        deploy_parameters : [],

        newKey: '',
        newValue: '',
        paramList: [],

        ifNameDuplicated: false,
    },
    components: {
    },
    methods: {
        gotoFile: function(pack)
        {
            window.location.href = pack.packsrc;
        },
        showDetail: function(pack)
        {
            var self = this;
            $.post('/api/pack/detail', {id: pack.name
                }, function(data) {
                    data = JSON.parse(data);
                    data.parameters = JSON.parse(data.parameters);
                    self.detail_pack = data;
                }
            );
            $('#detail_modal').modal('show');
        },
        gotoInstance: function(packname)
        {
            window.location.href = 'service_meta.html?filterKey=' + packname;
        },
        predeploy: function(pack)
        {
            this.deploy_pack = pack;
            this.deploy_parameters = pack.parameters;
            $('#deploypack').modal('show');
        },
        deploy: function()
        {
            if (this.app_name.trim() == "")
            {
                alert ("app_name cannot be empty");
                return;
            }
            
            for (key in this.deploy_pack.parameters)
            {
                this.deploy_parameters[key] = document.getElementById("deploy_parameters_" + key).value;
            }
            
            var req = new configuration_create_app_request({
                    'app_name': this.app_name.trim(),
                    'options': new create_app_options({
                        'app_type': this.deploy_pack.name,
                        'is_stateful': (this.deploy_pack.is_stateful=='true')?true:false,
                        'partition_count': parseInt(this.partition_count),
                        'replica_count': parseInt(this.replica_count),
                        'success_if_exist': (this.success_if_exist=='true')?true:false,
                        'envs': this.deploy_parameters
                    })
                });
                
            // alert (JSON.stringify(req));
            
            var client = new meta_sApp("http://"+localStorage['target_meta_server']);
            result = client.create_app({
                args: req,
                async: true,
                on_success: function (data){
                    data = new configuration_create_app_response(data);
                    //alert(JSON.stringify(data));
                    window.location.href = 'service_meta.html';
                },
                on_fail: function (xhr, textStatus, errorThrown) {}
            });

        },
        remove: function(packname)
        {
            $.post("/api/pack/del", { name: packname
                }, function(data){ 
                   location.reload(false); 
                }
            );
        },

        addKV: function()
        {
            var key = this.newKey && this.newKey.trim();
            var value = this.newValue && this.newValue.trim();
            if (!key) {
                return;
            }
            
            if (key.indexOf(';') != -1 || key.indexOf('"') != -1
             || value.indexOf(';') != -1 || value.indexOf('"') != -1)
            {
                alert("cannot contain ';' or '\"' in key/value");
                return;
            }
            
            this.paramList.push({key: key, value:value});
            this.newKey = '';        
            this.newValue = '';        
        },
        removeKV: function(kv)
        {
            this.paramList.$remove(kv);   
        },

        updatePackList: function()
        {
            var self = this;
            $.post("/api/pack/load", {
                }, function(data){
                    self.packList = JSON.parse(data);
                    for(index in self.packList)
                    {
                        pkg = self.packList[index];
                        pkg.parameters = JSON.parse(pkg.parameters);
                        pkg.imgsrc = "local/packages/" + pkg.name + "/" + pkg.icon_name;
                        pkg.packsrc = "local/packages/" + pkg.file_name;
                    }
                }   
            );
        }
    },
    ready: function ()
    {
        var self = this;
        self.updatePackList();    
        setInterval(function () {
            self.updatePackList();    
        }, 1000);
    }
});


