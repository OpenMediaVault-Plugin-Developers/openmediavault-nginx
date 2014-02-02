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
// require("js/omv/workspace/window/Form.js")
// require("js/omv/workspace/window/plugin/ConfigObject.js")
// require("js/omv/form/plugin/LinkedFields.js")
// require("js/omv/form/field/SharedFolderComboBox.js")
// require("js/omv/form/field/UserComboBox.js")

Ext.define("OMV.module.admin.service.nginx.window.Server", {
    extend   : "OMV.workspace.window.Form",
    requires : [
        "OMV.workspace.window.plugin.ConfigObject",
        "OMV.form.plugin.LinkedFields",
        "OMV.form.field.SharedFolderComboBox",
        "OMV.form.field.UserComboBox"
    ],

    plugins : [{
        ptype : "configobject"
    },{
        ptype        : "linkedfields",
        correlations : [{
            name : [
                "public_directory"
            ],
            conditions : [{
                name  : "use_public_directory",
                value : true
            }],
            properties : [
                "show",
                "!allowBlank",
                "!readOnly"
            ]
        },{
            name : [
                "server_name",
                "server_name_use_default_port"
            ],
            conditions : [{
                name  : "host_type",
                op    : ">",
                value : 0
            }],
            properties : [
                "show",
                "!allowBlank",
                "!readOnly"
            ]
        },{
            name : [
                "port"
            ],
            conditions : [{
                name  : "host_type",
                value : 1
            },{
                name  : "server_name_use_default_port",
                value : true
            }],
            properties : [
                "hide",
                "allowBlank",
                "readOnly"
            ]
        },{
            name : [
                "php_user"
            ],
            conditions : [{
                name  : "php_enable",
                value : true
            }],
            properties : [
                "!allowBlank",
                "!allowNone",
                "!readOnly",
                "show"
            ]
        }]
    }],

    rpcService   : "Nginx",
    rpcSetMethod : "set",

    height          : 600,
    hideResetButton : true,
    uuid            : null,

    getFormItems : function() {
        var me = this;

        return [{
            xtype : "fieldset",
            title : _("General"),
            items : [{
                xtype      : "checkbox",
                name       : "enable",
                fieldLabel : _("Enable"),
                checked    : true
            },{
                xtype      : "sharedfoldercombo",
                name       : "sharedfolderref",
                fieldLabel : _("Document root"),
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("The location needs to have at least read permissions for the user/group www-data")
                }]
            },{
                xtype      : "checkbox",
                name       : "use_public_directory",
                fieldLabel : _("Use public directory"),
                checked    : false,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Use if you serve your public files in a subfolder like public_html or public. This folder will be the document root. It also avoids the need to create an additional shared folder in such cases.")
                }]
            },{
                xtype      : "combo",
                name       : "public_directory",
                fieldLabel : _("Public directory"),
                queryMode  : "local",
                store      : Ext.create("Ext.data.ArrayStore", {
                    fields : [
                        "value"
                    ],
                    data   : [
                        [ "public_html" ],
                        [ "public" ],
                    ]
                }),
                displayField  : "value",
                valueField    : "value",
                allowBlank    : false,
                editable      : false,
                triggerAction : "all",
                value         : "public_html"
            },{
                xtype      : "combo",
                name       : "host_type",
                fieldLabel : _("Host type"),
                queryMode  : "local",
                store      : Ext.create("Ext.data.ArrayStore", {
                    fields : [
                        "value",
                        "text"
                    ],
                    data   : [
                        [ 0, _("Port") ],
                        [ 1, "ServerName" ],
                        [ 2, _("Both") ],
                    ]
                }),
                displayField  : "text",
                valueField    : "value",
                allowBlank    : false,
                editable      : false,
                triggerAction : "all",
                value         : 0
            },{
                xtype         : "numberfield",
                name          : "port",
                fieldLabel    : _("Port"),
                vtype         : "port",
                minValue      : 0,
                maxValue      : 65535,
                allowDecimals : false,
                allowNegative : false,
                value         : 8080,
                listeners     : {
                    "change" : function(field, newValue, oldValue) {
                        OMV.Rpc.request({
                            scope : me,
                            callback : function(id, success, response) {
                                if (success) {
                                    if (!response) {
                                        field.markInvalid("This port is already used!");
                                    }
                                }
                            },
                            rpcData : {
                                service : "Nginx",
                                method : "validatePort",
                                params : {
                                    uuid : me.uuid,
                                    port : newValue
                                }
                            }
                        });
                    }
                }
            },{
                xtype      : "textfield",
                name       : "server_name",
                fieldLabel : "ServerName",
                allowBlank : true,
                readOnly   : true,
                hidden     : true
            },{
                xtype      : "checkbox",
                name       : "server_name_use_default_port",
                fieldLabel : _("Use default port for ServerName"),
                checked    : true,
                readOnly   : true,
                hidden     : true,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Untick to host the ServerName with the portnumber specified in the field port.")
                }]
            }]
        },{
            xtype : "fieldset",
            title : "Options",
            items : [{
                xtype      : "checkbox",
                name       : "php_enable",
                fieldLabel : _("Enable PHP"),
                checked    : false
            },{
                xtype      : "usercombo",
                name       : "php_user",
                fieldLabel : _("User"),
                userType   : "normal",
                editable   : false,
                allowBlank : true,
                allowNone  : true,
                readOnly   : true,
                hidden     : true,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Set the user under which PHP scripts should be executed as.")
                }]
            }]
        },{
            xtype    : "fieldset",
            title    : "Options",
            layout   : "column",
            defaults : {
                columnWidth : 0.5,
                layout      : "form",
                border      : false,
            },
            items  : [{
                defaults : {
                    hideLabel      : true,
                    labelSeparator : ""
                },
                items : [{
                    xtype      : "checkbox",
                    name       : "autoindex",
                    boxLabel   : "Autoindex",
                    checked    : false
                }]
            },{
                defaults : {
                    hideLabel      : true,
                    labelSeparator : ""
                },
                items : [{
                    xtype      : "checkbox",
                    name       : "deny_htaccess",
                    boxLabel   : _("Don't serve .htaccess"),
                    checked    : false
                }]
            }]
        },{
            xtype : "fieldset",
            title : _("Extra options"),
            items : [{
                xtype      : "textarea",
                name       : "extra_options",
                fieldLabel : _("Extra options"),
                allowBlank : true
            }]
        }];
    }
});
