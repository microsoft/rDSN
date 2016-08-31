//Vue.config.debug = true;

//parameter parsing function
function getParameterByName(name) {
    name = name.replace(/[\[]/, "\\[").replace(/[\]]/, "\\]");
    var regex = new RegExp("[\\?&]" + name + "=([^&#]*)"),
        results = regex.exec(location.search);
    return results === null ? "" : decodeURIComponent(results[1].replace(/\+/g, " "));
}

var vm = new Vue({
    el: '#app',
    data:{
        appInfo: new configuration_list_apps_response(),
        appDetails: [],
        appDummy : new configuration_query_by_index_response(),
        updateTimer: 0,
        filterKey: '',
        
        AS_INVALID : 0,
        AS_AVAILABLE : 1,
        AS_CREATING : 2,
        AS_CREATE_FAILED : 3,
        AS_DROPPING : 4,
        AS_DROP_FAILED : 5,
        AS_DROPPED : 6,        
        AS_STATES : []
    },
    components: {
    },
    methods: {
        get_app : function(app_id)
        {
            var r = this.appDetails[app_id];
            if (r == undefined || r == null || r.partitions == null)
                return this.appDummy;
            else
                return r;
        },
        update: function()
        {
            var self = this;
            var client = new meta_sApp("http://"+localStorage['target_meta_server']);
            result = client.list_apps({
                args: new configuration_list_apps_request(),
                async: true,
                on_success: function (appdata){
                    try {
                        appdata = new configuration_list_apps_response(appdata);
                        self.appInfo = appdata;
                    }
                    catch(err) {
                        console.log(err);
                        return;
                    }

                    var app;
                    for (app = 0; app < self.appInfo.infos.length; ++app)
                    {
                        client.query_configuration_by_index({
                            args: new configuration_query_by_index_request({
                                'app_name': self.appInfo.infos[app].app_name,
                                'partition_indices': []
                            }),
                            async: true,
                            on_success: function (servicedata){
                                var is_stateful = false;
                                try {
                                    servicedata = new configuration_query_by_index_response(servicedata);
                                    console.log(JSON.stringify(servicedata));
                                    if (servicedata.app_id == 0)
                                        return;
                                    
                                    self.appDetails.$set(servicedata.app_id, servicedata);
                                    is_stateful = servicedata.is_stateful;
                                }
                                catch(err) {
                                    return;
                                }
                                
                                var partition;
                                for (partition = 0; partition < servicedata.partitions.length; ++partition)
                                {
                                    var par = servicedata.partitions[partition];
                                    par.membership = '';

                                    if(is_stateful)
                                    {
                                        if(par.primary != 'invalid address')
                                        {
                                            par.membership += 'P: ("' + par.primary.host + ':'+ par.primary.port  + '"), ';
                                            
                                        }
                                        else
                                        {
                                            par.membership += 'P: (), ';
                                        }

                                        par.membership += 'S: [';
                                        for (secondary in par.secondaries)
                                        {
                                            par.membership += '"' + par.secondaries[secondary].host + ':' + par.secondaries[secondary].port + '",'
                                        }
                                        par.membership += '],';

                                        par.membership += 'D: [';
                                        for (drop in par.last_drops)
                                        {
                                            par.membership += '"' + par.last_drops[drop].host +':'+ par.last_drops[drop].port + '",'
                                        }
                                        par.membership += ']';
                                    }
                                    else
                                    {
                                        par.membership += '[';
                                        for (worker in par.last_drops)
                                        {
                                            par.membership += '"' + par.last_drops[worker].host + ':' + par.last_drops[worker].port + '",'
                                        }
                                        par.membership += ']';
                                    }
                                }
                            },
                            on_fail: function (xhr, textStatus, errorThrown) {}
                        });
                    }
                },
                on_fail: function (xhr, textStatus, errorThrown) {}
            });
        },
        del: function (app_name)
        {
            var self = this;            
            var client = new meta_sApp("http://"+localStorage['target_meta_server']);
            result = client.drop_app({
                args: new configuration_drop_app_request({
                    'app_name': app_name,
                    'options': new drop_app_options ({'success_if_not_exist':false})
                }),
                async: true,
                on_success: function (data){
                    data = new configuration_drop_app_response(data);
                    //alert(JSON.stringify(data));
                },
                on_fail: function (xhr, textStatus, errorThrown) { alert("access remote server failed: " + textStatus) }
            });
        }
    },
    ready: function ()
    {
        var self = this;
        
        self.appDummy.partitions = [];
        
        self.AS_STATES[self.AS_INVALID] = "INVALID";
        self.AS_STATES[self.AS_AVAILABLE] = "AVAILABLE"; 
        self.AS_STATES[self.AS_CREATING] = "CREATING"; 
        self.AS_STATES[self.AS_CREATE_FAILED] = "CREATE_FAILED"; 
        self.AS_STATES[self.AS_DROPPING] = "DROPPING"; 
        self.AS_STATES[self.AS_DROP_FAILED] = "DROP_FAILED"; 
        self.AS_STATES[self.AS_DROPPED] = "DROPPED";

        self.filterKey = getParameterByName("filterKey");
        self.update(); 
        //query each machine their service state
        self.updateTimer = setInterval(function () {
            self.update(); 
        }, 3000);
    }
});

