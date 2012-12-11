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
    var is_showing_file = false;
    
    if (window.project_name){
	$("#main_header").html("#tags for " + project_name);
    }
    
    $("#searchbox").keyup(function(){
	var text = $(this).val();
	filter_comments(text);
    });
    
    $(".comment_href").click(function(event) {
	// It's sad, but basically what I want to do is very difficult with Chrome.  
        // I want to run the website from c:/foo/tags.html & access c:/bar/data.cpp
        // but there is no real way to do this due to (obvious) security issues.
        // Unless I run a browser.

	// event.preventDefault();
	
	// var file = $(this).attr('href');
	// $.get(file, function(data) {
	//     $("#fileview_pre").text(data);
	//     sh_highlightDocument();
	//     $("#div_fileview").show("fast");
	//     $("#span_filename").html(file);
	//     is_showing_file = true;
	// });
	
    });
    
    $(".tag_aref").click(function(event){
	var search = $("#searchbox")
	if (search.val() != "") { search.val(search.val() + " "); }
	search.val(search.val() + $(this).html())
	filter_comments(search.val());
	event.preventDefault();
    });
    
    $(".tag_aref").dblclick(function(event){
	var search = $("#searchbox")
	search.val($(this).html())
	filter_comments(search.val());
	event.preventDefault();
    });
    
    $("#help_link").click(function() {
	$("#div_help").show(200);
	event.preventDefault();
    });
    
    $("#help_close_link").click(function() {
	$("#div_help").hide(200);
	event.preventDefault();
    });
    
    $(document).keyup(function(event){
	if (event.which == 27) {
	    if (is_showing_file) {
		$("#div_fileview").hide("fast");
	    }
	    else
	    {
		// remove the last word
		var search = $("#searchbox")
		var words = $.trim(search.val());
		words = words.split(" ");
		words.pop();
		search.val($.trim(words.join(" ")))
		filter_comments(search.val());
	    }
	}
    });
    
});
