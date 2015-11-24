module.exports = function( el, class_string ){
	var classes = el.className;
	var classRegEx = new RegExp( '\\b'+ class_string +'\\b', 'ig' );
	var class_exists = !!classRegEx.exec( classes );
	el.className = class_exists?
		classes.replace( classRegEx, '' ):
		classes +' '+ class_string;
	return el;
};