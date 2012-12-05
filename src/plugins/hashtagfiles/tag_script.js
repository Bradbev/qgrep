function extract_trigrams_from_string(s) {
    var ret = {};
    for (var i = 0; i < s.length-2; i++) {
	ret[s.substring(i, i+3)] = true;
    }
    return ret;
}

function object_intersection(o1, o2) {
    var ret = {};
    for (var k in o1) {
	if (o2[k]) {
	    ret[k] = true;
	}
    }
    return ret;
}

function object_union(o1, o2) {
    var ret = {};
    for (var k in o1){
	ret[k] = o1[k];
    }
    for (var k in o2){
	ret[k] = o2[k];
    }
    return ret;
}


function search_trigrams(to_find) {
    to_find = to_find.toLowerCase();
    var sret = "";
    
    var active_grams = null;
    for (var key in extract_trigrams_from_string(to_find)) {
	var g = trigrams[key];
	sret = sret + key + ":";
	if (!g) { return {}; }
	
	if (active_grams == null) { 
	    active_grams = g; 
	} else {
	    active_grams = object_intersection(active_grams, g);
	}
    }
    var ret = {};
    if (active_grams) {
	for (var k in active_grams) {
	    if (comments[k].toLowerCase().indexOf(to_find) != -1) {
		ret[k] = true;
	    }
	}
    }
    return ret;
}

function filter_comments(to_find) {
    var words = to_find.split(" ");
    var visible = search_trigrams(words[0]);
    for (var i = 1; i < words.length; i++){
	var word = words[i];
	if (word.length >= 3) {
	    visible = object_intersection(visible, search_trigrams(word));
	}
    }
    
    if (to_find.length < 3) { visible = null; }
    if (visible) {
	for (var i = 1; i < comments.length + 1; i++) {
	    if (visible[i]) {
		$(".comment#" + i).show();
	    } else {
		$(".comment#" + i).hide();
	    }
	}
    } else {
	for (var i = 1; i < comments.length + 1; i++) {
	    $(".comment#" + i).show();
	}
    }
}

$(document).ready(function(){
    $("#main_header").html("#Tags for " + project_name);
    
    $("#searchbox").keyup(function(){
	var text = $(this).val();
	filter_comments(text);
    });
    
    $(".comment_aref").click(function(event){
	//event.preventDefault();
    });
    
    $(".tag_aref").click(function(event){
	var search = $("#searchbox")
	if (search.val() != "") { search.val(search.val() + " "); }
	search.val(search.val() + $(this).html())
	filter_comments(search.val());
	event.preventDefault();
    });
});
