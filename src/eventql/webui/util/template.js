/**
 * Copyright (c) 2017 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Laura Schlimmer <laura@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */

TemplateUtil = this.TemplateUtil || (function() {
  var enable_html5_templates = ("content" in document.createElement("template"));
  var enable_html5_importnode = 'importNode' in document;

  try {
    document.importNode(document.createElement('div'));
  } catch (e) {
    enable_html5_importnode = false;
  }

  function importNodeFallback(node, deep) {
    var a, i, il, doc = document;

    switch (node.nodeType) {

      case document.DOCUMENT_FRAGMENT_NODE:
        var new_node = document.createDocumentFragment();
        while (child = node.firstChild) {
          new_node.appendChild(node);
        }
        return new_node;

      case document.ELEMENT_NODE:
        var new_node = doc.createElementNS(node.namespaceURI, node.nodeName);
        if (node.attributes && node.attributes.length > 0) {
          for (i = 0, il = node.attributes.length; i < il; i++) {
            a = node.attributes[i];
            try {
              new_node.setAttributeNS(
                  a.namespaceURI,
                  a.nodeName,
                  node.getAttribute(a.nodeName));
            } catch (err) {}
          }
        }
        if (deep && node.childNodes && node.childNodes.length > 0) {
          for (i = 0, il = node.childNodes.length; i < il; i++) {
            new_node.appendChild(
                importNodeFallback(node.childNodes[i],
                deep));
          }
        }
        return new_node;

      case document.TEXT_NODE:
      case document.CDATA_SECTION_NODE:
      case document.COMMENT_NODE:
        return doc.createTextNode(node.nodeValue);

    }
  }

  var getTemplate = function(template_id) {
    var template_selector = "#" + template_id;

    var template = document.querySelector(template_selector);

    if (!template) {
      return null;
    }

    var content;
    if (enable_html5_templates) {
      content = template.content;
    } else {
      content = document.createDocumentFragment();
      var children = template.children;

      for (var j = 0; j < children.length; j++) {
        content.appendChild(children[j].cloneNode(true));
      }
    }

    if (enable_html5_importnode) {
      return document.importNode(content, true);
    } else {
      return importNodeFallback(content, true);
    }
  };

  return {
    getTemplate: getTemplate
  };
})();

