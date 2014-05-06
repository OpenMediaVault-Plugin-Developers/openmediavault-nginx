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
                "server_name"
            ],
            conditions : [{
                name  : "host_type",
                value : "name"
            }],
            properties : [
                "show",
                "!allowBlank",
                "!readOnly"
            ]
        },{
            name : [
                "port",
                "port_default_server",
            ],
            conditions : [{
                name  : "ssl_force",
                value : true
            }],
            properties : [
                "allowBlank",
                "readOnly",
            ]
        },{
            name : [
                "sslcertificateref",
                "ssl_port",
                "ssl_port_default_server",
                "ssl_force"
            ],
            conditions : [{
                name  : "ssl_enable",
                value : true
            }],
            properties : [
                "show",
                "!allowBlank",
                "!allowNone",
                "!readOnly"
            ]
        },{
            name : [
                "php_fpm_pool_socket"
            ],
            conditions : [{
                name  : "php_enable",
                value : true
            }],
            properties : [
                "show"
            ]
        },{
            name : [
                "php_user",
                "php_display_errors",
                "php_html_errors",
                "php_max_execution_time",
                "php_memory_limit",
                "php_post_max_size",
                "php_upload_max_filesize"
            ],
            conditions : [{
                name  : "php_enable",
                value : true
            },{
                name  : "php_fpm_pool_socket",
                value : ""
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
                        [ "port", _("Port") ],
                        [ "name", _("Name-based") ]
                    ]
                }),
                displayField  : "text",
                valueField    : "value",
                allowBlank    : false,
                editable      : false,
                triggerAction : "all",
                value         : "port"
            },{
                xtype      : "textfield",
                name       : "server_name",
                fieldLabel : _("Server name"),
                allowBlank : true,
                readOnly   : true,
                hidden     : true
            },{
                xtype         : "numberfield",
                name          : "port",
                fieldLabel    : _("Port"),
                vtype         : "port",
                minValue      : 0,
                maxValue      : 65535,
                allowDecimals : false,
                allowNegative : false,
                value         : 80
            },{
                xtype      : "checkbox",
                name       : "port_default_server",
                fieldLabel : _("Default server"),
                checked    : false,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Set the server as the default one to be served if no server name matches on the selected port.")
                }]
            }]
        },{
            xtype : "fieldset",
            title : "SSL",
            items : [{
                xtype      : "checkbox",
                name       : "ssl_enable",
                fieldLabel : _("Enable SSL"),
                checked    : false,
                listeners  : {
                    "change" : function(field, newValue) {
                        var sslForceField = me.findField("ssl_force");
                        sslForceField.setValue(false);
                    }
                }
            },{
                xtype         : "numberfield",
                name          : "ssl_port",
                fieldLabel    : _("Port"),
                vtype         : "port",
                minValue      : 0,
                maxValue      : 65535,
                allowDecimals : false,
                allowNegative : false,
                value         : 443
            },{
                xtype      : "checkbox",
                name       : "ssl_port_default_server",
                fieldLabel : _("Default server"),
                checked    : false,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Set the server as the default one to be served if no server name matches on the selected port.")
                }]
            },{
                xtype      : "certificatecombo",
                name       : "sslcertificateref",
                fieldLabel : _("Certificate"),
                allowNone  : true,
                allowBlank : true,
                readOnly   : true,
                hidden     : true,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("The SSL certificate.")
                }]
            },{
                xtype      : "checkbox",
                name       : "ssl_force",
                fieldLabel : _("Force SSL"),
                checked    : false
            }]
        },{
            xtype : "fieldset",
            title : "PHP",
            items : [{
                xtype      : "checkbox",
                name       : "php_enable",
                fieldLabel : _("Enable PHP"),
                checked    : false
            },{
                xtype      : "textfield",
                name       : "php_fpm_pool_socket",
                fieldLabel : _("PHP-FPM socket"),
                hidden     : true,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Set this if you want to use a custom FPM socket. Will disable all other PHP options.")
                }]
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
            },{
                xtype      : "checkbox",
                name       : "php_display_errors",
                fieldLabel : _("Display errors"),
                checked    : false
            },{
                xtype      : "checkbox",
                name       : "php_html_errors",
                fieldLabel : _("HTML errors"),
                checked    : true,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Use HTML tags in error messages."),
                }]
            },{
                xtype      : "numberfield",
                name       : "php_max_execution_time",
                fieldLabel : _("Max execution time (s)"),
                minValue   : 0,
                value      : 30,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Set the max execution time of a script. Using the value 0 means no limit."),
                }]
            },{
                xtype      : "numberfield",
                name       : "php_memory_limit",
                fieldLabel : _("Memory limit (MB)"),
                minValue   : -1,
                value      : 128,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("This setting should be higher than the post max size. Setting a value of -1 makes it unlimited."),
                }],
                validator : function(value) {
                    var otherField = me.findField("php_post_max_size");

                    if (value != -1 && value < otherField.getValue())
                        return "Value should be higher than POST max size";

                    return true;
                }
            },{
                xtype      : "numberfield",
                name       : "php_post_max_size",
                fieldLabel : _("Max POST size (MB)"),
                minValue   : 1,
                value      : 8,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("This setting affects the max upload filesize and should preferably be higher."),
                }],
                validator : function(value) {
                    var otherField = me.findField("php_upload_max_filesize");

                    if (value < otherField.getValue())
                        return "Value should be higher than max upload filesize";

                    return true;
            }
            },{
                xtype      : "numberfield",
                name       : "php_upload_max_filesize",
                fieldLabel : _("Max upload filesize (MB)"),
                minValue   : 1,
                value      : 2
            }]
        },{
            xtype : "fieldset",
            title : _("Options"),
            items : [{
                border   : false,
                layout   : "column",
                defaults : {
                    border      : false,
                    columnWidth : 0.5,
                    layout      : "form"
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
                xtype      : "combo",
                name       : "large_client_header_buffers",
                fieldLabel : _("Header buffer size"),
                queryMode  : "local",
                store      : Ext.create("Ext.data.ArrayStore", {
                    fields : [
                        "value",
                        "text"
                    ],
                    data   : [
                        [ 8, "8k" ],
                        [ 16, "16k" ],
                        [ 32, "32k" ],
                    ]
                }),
                displayField  : "text",
                valueField    : "value",
                allowBlank    : false,
                editable      : false,
                triggerAction : "all",
                value         : 8
            }]
        },{
            xtype : "fieldset",
            title : _("Extra options"),
            items : [{
                xtype      : "textarea",
                name       : "extra_options",
                fieldLabel : _("Extra options"),
                allowBlank : true,
                plugins    : [{
                    ptype : "fieldinfo",
                    text  : _("Use the variable $root_path to insert the document root."),
                }]
            }]
        }];
    }
});
