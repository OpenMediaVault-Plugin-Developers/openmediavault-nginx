/**
 * Copyright (C) 2014 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/grid/Panel.js")
// require("js/omv/data/Store.js")
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")
// require("js/omv/module/admin/service/nginx/window/Server.js")

Ext.define("OMV.module.admin.service.nginx.Servers", {
    extend   : "OMV.workspace.grid.Panel",
    requires : [
        "OMV.data.Store",
        "OMV.data.Model",
        "OMV.data.proxy.Rpc",
        "OMV.module.admin.service.nginx.window.Server"
    ],

    hidePagingToolbar : true,

    columns : [{
        header    : _("UUID"),
        hidden    : true,
        dataIndex : "uuid"
    },{
        header    : _("Enabled"),
        sortable  : true,
        dataIndex : "enable",
        align     : "center",
        width     : 80,
        resizable : false,
        renderer  : OMV.util.Format.booleanIconRenderer(
            "switch_on.png", "switch_off.png")
    },{
        header    : _("Root"),
        flex      : 1,
        sortable  : true,
        dataIndex : "root_full_path"
    },{
        header    : _("URL"),
        flex      : 1,
        sortable  : true,
        dataIndex : "urls",
        renderer  : function(urls) {
            var presentation = "";

            for (var i = 0, j = urls.length; i < j; i++) {
                presentation += urls[i].replace(/!domain!/g, window.location.hostname);

                if (i < j - 1)
                    presentation += ", ";
            }

            return presentation;
        }
    }],

    initComponent : function() {
        var me = this;

        Ext.apply(me, {
            store : Ext.create("OMV.data.Store", {
                autoload   : true,
                remoteSort : false,
                model      : OMV.data.Model.createImplicit({
                    idProperty   : "uuid",
                    totalPoperty : "total",
                    fields       : [
                        { name : "uuid" },
                        { name : "enable" },
                        { name : "root_full_path" },
                        { name : "urls" }
                    ]
                }),
                proxy : {
                    type    : "rpc",
                    rpcData : {
                        "service" : "Nginx",
                        "method"  : "getList"
                    }
                }
            })
        });

        me.doReload();

        me.callParent(arguments);
    },

    onAddButton : function() {
        var me = this;

        Ext.create("OMV.module.admin.service.nginx.window.Server", {
            title        : _("Add server"),
            listeners    : {
                scope  : me,
                submit : function() {
                    me.doReload();
                }
            }
        }).show();
    },

    onEditButton : function() {
        var me = this;
        var record = me.getSelected();

        Ext.create("OMV.module.admin.service.nginx.window.Server", {
            rpcGetMethod : "get",
            title        : _("Edit server"),
            uuid         : record.get("uuid"),
            listeners    : {
                scope  : me,
                submit : function() {
                    me.doReload();
                }
            }
        }).show();
    },

    doDeletion : function(record) {
        var me = this;

        OMV.Rpc.request({
            scope : me,
            callback : me.onDeletion,
            rpcData : {
                service : "Nginx",
                method : "delete",
                params : {
                    uuid : record.get("uuid")
                }
            }
        });
    }

});

OMV.WorkspaceManager.registerPanel({
    id        : "servers",
    path      : "/service/nginx",
    text      : _("Servers"),
    position  : 20,
    className : "OMV.module.admin.service.nginx.Servers"
});
