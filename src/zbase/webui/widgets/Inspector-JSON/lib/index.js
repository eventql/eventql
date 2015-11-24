var typeOf = require('type-of');
var extend = require('extend');
var Delegate = require('dom-delegate');
var store = require('store');

// a bad toggleClass method
var toggleClass = require('./utils/toggle_class.js');

// a possibly good string escaping method
var HTMLEscape = require('./utils/html_escape.js');

var InspectorJSON = function( params ){

	params = params || {};
	var defaults = {
		element: 'body',
		debug: false,
		collapsed: true,
		url: location.pathname
	};
	
	params = extend( defaults, params );
	// if the supplied element isn't an element, try to select it by ID
	if( typeOf( params.element ) !== 'element' ) params.element = document.getElementById( params.element );
	
	var collapse_states = store.get( params.url +':inspectorJSON/collapse_states' ) || {};
	this.el = params.element;
	this.el.className += ' inspector-json viewer';
	
	// Delegate the click event for collapse/uncollapse of JSON levels
	this.event_delegator = new Delegate( this.el );
	this.event_delegator.on( 'click', 'li.object > a, li.array > a', function( e ){
		e.preventDefault();
		var parent = this.parentNode;
		var path = parent.getAttribute('data-path');
		toggleClass( parent, 'collapsed' );
		if( !/\bcollapsed\b/ig.exec( parent.className ) ){
			collapse_states[path] = true;
		}
		else {
			delete collapse_states[path];
		}
		store.set( params.url +':inspectorJSON/collapse_states', collapse_states );
	});
	
	// Create the JSON Viewer on the specified element
	this.view = function( json ){
		
		var start, end;

		if( params.debug ){
			start = new Date().getTime();
		}
		
		// Create markup from a value
		var processItem = function( item, parent, key, path ){
			
			var type = typeOf( item );
			var parent_type = typeOf( parent );
			var markup = '';
			
			// Create a string representation of the JSON path to this value
			if( parent_type === 'array' ){
				path += '['+ key +']';
			}
			else if( parent_type === 'object' ){
				path += '.'+ key;
			}
			else {
				path = key || 'this';
			}
			
			// Start the <li>
			if( parent ){
				markup += ( collapse_states[path] || !params.collapsed || ( type !== 'object' && type !== 'array' ) ) ?
					'<li class="'+ type +'" data-path="'+ path +'">' :
					'<li class="'+ type +' collapsed" data-path="'+ path +'">';
			}
			
			// Generate markup by value type. Recursion for arrays and objects.
			if( type === 'object' ){
				if( key ){
					markup += '<a href="#toggle"><strong>'+ key + '</strong></a>';
				}
				markup += '<ul>';
				for( key in item ){
					markup += processItem( item[key], item, key, path );
				}
				markup += '</ul>';
			}
			else if( type === 'array' ){
				if( key ){
					markup += '<a href="#toggle"><strong>'+ key +'</strong></a>Array('+ item.length +')';
				}
				markup += '<ol>';
				for( var i in item ){
					markup += processItem( item[i], item, i, path );
				}
				markup += '</ol>';
			}
			else if( type === 'string' ){
				markup += '<strong>'+ key + '</strong><span>"'+ HTMLEscape( item ) +'"</span>';
			}
			else if( type === 'number' ){
				markup += '<strong>'+ key + '</strong><var>'+ item.toString() +'</var>';
			}
			else if( type === 'boolean' ){
				markup += '<strong>'+ key + '</strong><em>'+ item.toString() + '</em>';
			}
			else if( type === 'null' ){
				markup += '<strong>'+ key + '</strong><i>null</i>';
			}
			
			// End the </li>
			if( parent ){
				markup += '</li>';
			}
			
			return markup;
		
		};
		
		if( typeOf( json ) === 'string' ){
			json = JSON.parse( json );
		}
		
		var root = processItem( json );
		this.el.innerHTML = root;
		
		if( params.debug ){
			end = new Date().getTime();
			console.log('Inspector JSON: Rendered in '+ ( end - start ) +'ms');
		}
				
	};
	
	// LEAVE NO EVIDENCE!!!
	this.destroy = function(){
		
		this.event_delegator.off();
		this.el.innerHTML = '';
		
	};

	// if json is supplied, view it immediately
	if( !params.json ){
		try {
			params.json = JSON.parse( this.el.textContent || this.el.innerText );
		}
		catch( err ){
			if( params.debug ){
				console.log('Inspector JSON: Element contents are not valid JSON');
			}
		}
	}
	if( params.json ){
		this.view( params.json );
	}
	
};

module.exports = InspectorJSON;