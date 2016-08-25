//Vue.config.debug = true;

var vm = new Vue({
    el: '#app',
    data:{
        nodeList: new configuration_list_nodes_response(),
        nodeInfo: [],
        updateTimer: 0,
    },
    components: {
    },
    methods: {
        update: function()
        {
            var self = this;
            var client = new meta_sApp("http://"+localStorage['target_meta_server']);
            result = client.list_nodes({
                args: new configuration_list_nodes_request({
                    'node_status': 'NS_INVALID'
                }),
                async: true,
                on_success: function (nodedata){
                    try {
                        nodedata = new configuration_list_nodes_response(nodedata);
                        // console.log(JSON.stringify(nodedata));
                        self.$set('nodeList', nodedata);
                    }
                    catch(err) {
                    }
                    
                    var nodeIndex = 0;
                    for (nodeIndex = 0; nodeIndex < self.nodeList.infos.length; ++nodeIndex)
                    {   
                        (function (nidx) {
                            client.query_configuration_by_node({
                                args: new configuration_query_by_node_request({
                                    'node': new rpc_address({host:self.nodeList.infos[nidx].address.host,port:self.nodeList.infos[nidx].address.port})
                                }),
                                async: true,
                                on_success: function (servicedata){
                                    try {
                                        servicedata = new configuration_query_by_node_response(servicedata);
                                        // console.log(JSON.stringify(servicedata));
                                        self.nodeInfo.$set(nidx, servicedata);
                                    }
                                    catch(err) {
                                        return;
                                    }
                                    
                                    var host_node = self.nodeList.infos[nidx].address.host + ':' + self.nodeList.infos[nidx].address.port;
                                    
                                    var index = 0;
                                    for (index = 0; index < servicedata.partitions.length; ++index)
                                    {
                                        var par = servicedata.partitions[index];
                                        par.role = '';
                                        par.working_point = '';

                                        var addressList = {};
                                        addressList['primary'] = par.config.primary.host+':'+par.config.primary.port;
                                        addressList['secondaries'] = [];
                                        for (secondary in par.config.secondaries)
                                        {
                                            addressList['secondaries'][secondary] = par.config.secondaries[secondary].host +':'+ par.config.secondaries[secondary].port;
                                        } 
                                        addressList['last_drops'] = [];
                                        for (drop in par.config.last_drops)
                                        {
                                            addressList['last_drops'][drop] = par.config.last_drops[drop].host +':'+ par.config.last_drops[drop].port;
                                        }
                                        
                                        if(par.info.is_stateful==true)
                                        {
                                            //stateful service
                                            if (host_node == addressList.primary) 
                                            {
                                                par['role'] = 'primary';
                                            }
                                            else if (addressList.secondaries.indexOf(host_node) != -1)
                                            {
                                                par['role'] = 'secondary';
                                            }
                                            else if (addressList.last_drops.indexOf(host_node) != -1)
                                            {
                                                par['role'] = 'drop';
                                            }
                                            else
                                                par['role'] = 'undefined';
                                        }
                                        else
                                        {
                                            par['role'] = 'worker';
                                            par['working_point'] = addressList.last_drops[addressList.secondaries.indexOf(host_node)];
                                        }
                                    }
                                },
                                on_fail: function (xhr, textStatus, errorThrown) {}
                            });
                        })(nodeIndex);
                    }
                },
                on_fail: function (xhr, textStatus, errorThrown) {}
            });

        },
        del: function (address, role, gpid)
        {
            /*
            var self = this;
            console.log(((role!='')?'replica.':'daemon.') + "kill_partition " + gpid.app_id + " " + gpid.pidx);
            var client = new cliApp("http://"+localStorage['target_meta_server']);
            result = client.call({
                    args: new command({
                    cmd: ((role!='')?'replica.':'daemon.') + "kill_partition",
                    arguments: [gpid.app_id,gpid.pidx]
                }),
                async: true,
                on_success: function (data){
                    console.log(data);
                },
                on_fail: function (xhr, textStatus, errorThrown) {}
            });
            */
            alert('This function not available now');
        }
    },
    ready: function ()
    {
        var self = this;
        self.update(); 
        //query each machine their service state
        self.updateTimer = setInterval(function () {
            self.update(); 
        }, 3000);
    }
});

