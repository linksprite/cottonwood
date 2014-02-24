// JavaScript Document
 //快速回帖引用
    function fnQuickReplyQuote(obj, bIncludeQuote) {
        var iQuoteMaxLength = 400; // 最大引用长度
        var oPost = obj.parentNode.parentNode.parentNode.parentNode.parentNode.parentNode.parentNode.firstChild;
        var oReplyTextBox = document.getElementsByName("replycontent")[0];
        if (oPost != undefined && oReplyTextBox != undefined) {
            var oInfoArea = oPost.getElementsByTagName('td')[0];
            var oMainPostArea = oPost.getElementsByTagName('td')[1];
            var strUserName = oInfoArea.innerHTML.match(/【.+】/im)[0] + oInfoArea.getElementsByTagName('font')[0].innerHTML;         
            var strPostContent = oMainPostArea.getElementsByTagName('font')[0].innerHTML;
			strPostContent = strPostContent.replace(/<a\shref=(.+?)<\/a>/igm,"").replace(/<font\scolor=(.+?)><br><br>(.+?)<\/font>/igm,"").replace(/<center><\/center>/igm,"").replace(/<BR>/igm,"\n");
			strPostContent = strPostContent.replace(/\n\n\n/igm,"\n").replace(/\n\n/igm,"\n").replace(/&nbsp;/igm," ");


strPostContent = strPostContent.toLowerCase();
            strPostContent = (strPostContent.length > iQuoteMaxLength) ? (strPostContent.substring(0, iQuoteMaxLength) + "......") : (strPostContent);
            oReplyTextBox.value += "回复" + strUserName + "\n";

            if (bIncludeQuote) {
                oReplyTextBox.value += strPostContent + "\n";
            }
            oReplyTextBox.value += "-----------------------------------------------------------------------\n\n";
            var iTextBoxLength = oReplyTextBox.value.length;
            if(oReplyTextBox.createTextRange) {
                        var range = oReplyTextBox.createTextRange();
                        range.move('character', iTextBoxLength);
                        range.select();
                        oReplyTextBox.focus();
                }else{
                                if(oReplyTextBox.selectionStart) {
                        oReplyTextBox.focus();
                        oReplyTextBox.setSelectionRange(iTextBoxLength, iTextBoxLength);
                        }else{
                                oReplyTextBox.focus();
                        }
                }
        }
    }
	function fnTest(obj) {  
	        var oPost = obj.parentNode.parentNode.parentNode.parentNode.firstChild;  
			 if (oPost != undefined )
			 {
				  var oInfoArea = oPost.getElementsByTagName('td')[0];
    	        	var oMainPostArea = oPost.getElementsByTagName('td')[1];

				alert (oInfoArea.innerHTML);
			
			 }
			 else
			 {
			  	alert("no");
			 }   
	}
	//快速图片引用
    function fnQuickImageQuote(obj,img_file,img_desc) {       
        var iQuoteMaxLength = 400; // 最大引用长度
        var oPost = obj.parentNode.parentNode.parentNode.parentNode.parentNode.firstChild;  
        var oReplyTextBox = document.getElementsByName("replycontent")[0];
        if (oReplyTextBox != undefined) {
            var oInfoArea = oPost.getElementsByTagName('td')[0];
            var oMainPostArea = oPost.getElementsByTagName('td')[1];
            var strUserName = oInfoArea.innerHTML.match(/【.+】/im)[0] + oInfoArea.getElementsByTagName('font')[0].innerHTML;     
    
            var strPostContent = oMainPostArea.getElementsByTagName('font')[0].innerHTML.replace(/<BR>/igm,"\n");
            strPostContent = strPostContent.replace(/\n\n\n/igm,"\n").replace(/\n\n/igm,"\n").replace(/&nbsp;/igm," ");

			
            strPostContent = (strPostContent.length > iQuoteMaxLength) ? (strPostContent.substring(0, iQuoteMaxLength) + "......") : (strPostContent);
            oReplyTextBox.value += "引用图片" + strUserName + "\n";


            oReplyTextBox.value += "-----------------------------------------------------------------------\n";
			oReplyTextBox.value += "<center><a href=./bbs_upload/"+ img_file+" target='_blank'><img src=./bbs_upload/" + img_file +" border=0></a>\n <font color=green>("+img_desc +")</font> </center>\n";
            var iTextBoxLength = oReplyTextBox.value.length;
            if(oReplyTextBox.createTextRange) {
                        var range = oReplyTextBox.createTextRange();
                        range.move('character', iTextBoxLength);
                        range.select();
                        oReplyTextBox.focus();
                }else{
                                if(oReplyTextBox.selectionStart) {
                        oReplyTextBox.focus();
                        oReplyTextBox.setSelectionRange(iTextBoxLength, iTextBoxLength);
                        }else{
                                oReplyTextBox.focus();
                        }
                }
        }
    }

