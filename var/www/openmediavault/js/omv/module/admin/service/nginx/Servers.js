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
// require("js/omv/data/Model.js")
// require("js/omv/data/proxy/Rpc.js")
// require("js/omv/data/Store.js")
// require("js/omv/module/admin/service/nginx/window/Server.js")
// require("js/omv/module/admin/service/nginx/window/Log.js")

Ext.define("OMV.module.admin.service.nginx.Servers", {
    extend   : "OMV.workspace.grid.Panel",
    requires : [
        "OMV.data.Model",
        "OMV.data.proxy.Rpc",
        "OMV.data.Store",
        "OMV.module.admin.service.nginx.window.Log",
        "OMV.module.admin.service.nginx.window.Server"
    ],

    accessLogButtonText : _("Access log"),
    errorLogButtonText  : _("Error log"),
    reloadOnActivate    : true,

    columns : [{
        header    : _("UUID"),
        hidden    : true,
        dataIndex : "uuid"
    },{
        xtype     : "booleaniconcolumn",
        header    : _("Enabled"),
        sortable  : true,
        dataIndex : "enable",
        align     : "center",
        width     : 80,
        resizable : false,
        trueIcon  : "switch_on.png",
        falseIcon : "switch_off.png"
    },{
        xtype     : "booleaniconcolumn",
        header    : "SSL",
        sortable  : true,
        dataIndex : "ssl_enable",
        align     : "center",
        width     : 60,
        resizable : false,
        trueIcon  : "switch_on.png",
        falseIcon : "switch_off.png"
    },{
        xtype     : "booleaniconcolumn",
        header    : "PHP",
        sortable  : true,
        dataIndex : "php_enable",
        align     : "center",
        width     : 60,
        resizable : false,
        trueIcon  : "switch_on.png",
        falseIcon : "switch_off.png"
    },{
        xtype     : "booleaniconcolumn",
        header    : _("Default"),
        sortable  : true,
        dataIndex : "port_default_server",
        align     : "center",
        width     : 60,
        resizable : false,
        trueIcon  : "switch_on.png",
        falseIcon : "switch_off.png"
    },{
        xtype     : "booleaniconcolumn",
        header    : _("Default (SSL)"),
        sortable  : true,
        dataIndex : "ssl_port_default_server",
        align     : "center",
        width     : 80,
        resizable : false,
        trueIcon  : "switch_on.png",
        falseIcon : "switch_off.png"
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
            var value = "";

            for (var i = 0; i < urls.length; i++)
                urls[i] = Ext.String.format("<a href='{0}' target='_blank'>{0}</a>", urls[i]
                    .replace("!domain!", window.location.hostname));

            value = urls.join(", ");

            return value;
        }
    }],

    store : Ext.create("OMV.data.Store", {
        autoload   : true,
        remoteSort : false,
        model      : OMV.data.Model.createImplicit({
            idProperty   : "uuid",
            totalPoperty : "total",
            fields       : [
                { name : "uuid" },
                { name : "enable" },
                { name : "log_enable" },
                { name : "ssl_enable" },
                { name : "php_enable" },
                { name : "port_default_server" },
                { name : "ssl_port_default_server" },
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
    }),

    getTopToolbarItems : function() {
        var me = this;
        var items = me.callParent(arguments);

        Ext.Array.push(items, [{
            xtype : "tbseparator"
        },{
            id       : me.getId() + "-access-log",
            xtype    : "button",
            text     : me.accessLogButtonText,
            handler  : Ext.Function.bind(me.onViewLogButton, me, [ "access" ]),
            scope    : me,
            disabled : true,
            selectionConfig : {
                minSelections : 1,
                maxSelections : 1,
                enabledFn     : me.logButtonEnabled
            }
        },{
            id       : me.getId() + "-error-log",
            xtype    : "button",
            text     : me.errorLogButtonText,
            handler  : Ext.Function.bind(me.onViewLogButton, me, [ "error" ]),
            scope    : me,
            disabled : true,
            selectionConfig : {
                minSelections : 1,
                maxSelections : 1,
                enabledFn     : me.logButtonEnabled
            }
        }]);

        return items;
    },

    logButtonEnabled : function(button, records) {
        var record = records[0];

        return record.get("log_enable");
    },

    onAddButton : function() {
        var me = this;

        Ext.create("OMV.module.admin.service.nginx.window.Server", {
            title        : _("Add server"),
            uuid         : OMV.UUID_UNDEFINED,
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
            scope    : me,
            callback : me.onDeletion,
            rpcData  : {
                service : "Nginx",
                method  : "delete",
                params  : {
                    uuid : record.get("uuid")
                }
            }
        });
    },

    onViewLogButton : function(logType) {
        var me = this;
        var record = me.getSelected();

        Ext.create("OMV.module.admin.service.nginx.window.Log", {
            uuid    : record.get("uuid"),
            logType : logType
        }).show();
    }
});

OMV.WorkspaceManager.registerPanel({
    id        : "servers",
    path      : "/service/nginx",
    text      : _("Servers"),
    position  : 20,
    className : "OMV.module.admin.service.nginx.Servers"
});
