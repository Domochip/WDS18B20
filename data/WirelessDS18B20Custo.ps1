#Script to prepare Web files
# - Integrate customization of the application
# - GZip of resultant web files
# - and finally convert compressed web files to C++ header in PROGMEM

#List here web files specific to this project/application
$specificFiles="",""

#list here files that need to be merged with Common templates
$applicationName="WirelessDS18B20"
$shortApplicationName="WDS18B20"
$templatesWithCustoFiles=@{
    #---Status.html---
    "status.html"=@{
        ApplicationName=$applicationName
        ;
        ShortApplicationName=$shortApplicationName
        ;
        HTMLContent=@'
        <h2 class="content-subhead">OneWire<span id="l3"><h6 style="display:inline"><b> Loading...</b></h6></span></h2>
        <div id="AllSensors"></div>
'@
        ;
        HTMLScriptInReady=@'
            $.getJSON("/gc1", function(GC1){

                var buses = [];
                for(i=0;i<GC1.n;i++) buses.push(i);

                $(buses).each(function(bus){
                    $("#AllSensors").append("<h3>Bus "+bus+"</h3></div>");
                    $("#AllSensors").append("<div id='bus"+bus+"' class='pure-controls'></div>");
                    
                    $.getJSON("/getL?bus"+bus,function(SList){
                        $(SList.TemperatureSensorList).each(function(index,RomCode){
                            $.getJSON("/getT?bus"+bus+"="+RomCode, function(temp){
                                $("#bus"+bus).append(RomCode+" : <a class='pure-button pure-button-primary' href='/getT?bus"+bus+"="+RomCode+"'>"+temp.Temperature+"°C</a><br>");
                            });
                        });
                    });
                });
                $("#l3").fadeOut();
            })
            .fail(function(){
                $("#l3").html('<h4 style="display:inline;color:red;"><b> Failed</b></h4>');
            });
'@
    }
    ;
    #---config.html---
    "config.html"=@{
        ApplicationName=$applicationName
        ;
        ShortApplicationName=$shortApplicationName
        ;
        HTMLContent=@'
        <h2 class="content-subhead">OneWire<span id="l1"><h6 style="display:inline"><b> Loading...</b></h6></span></h2>
        <form class="pure-form pure-form-aligned" id='f1'>
            <fieldset>

            <div class="pure-control-group">
                <label for="n">number Of OW Bus</label>
                <input type='number' id='n' name='n' min='1'><span id='e'></span>
            </div>
            <div id="buses"></div>
        
                <div class="pure-controls">
                    <input type='submit' value='Save' class="pure-button pure-button-primary" disabled>
                </div>
            </fieldset>
        </form>
        <span id='r1'></span>
'@
        ;
        HTMLScript=@'
        function onNChange(){
            for(i=0;i<$("#n").val();i++){
                $("#b"+i+"i").prop("disabled",false).prop("required",true);
                $("#b"+i+"o").prop("disabled",false).prop("required",true);
                $("#b"+i).show();
            }
            for(i=$("#n").val();i<$("#n").prop("max");i++){
                $("#b"+i).hide();
                $("#b"+i+"i").prop("disabled",true).prop("required",false);
                $("#b"+i+"o").prop("disabled",true).prop("required",false);
            }
        };
        $("#n").change(onNChange);

        $("#f1").submit(function(event){
            $("#r1").html("Saving Configuration...");
            $.post("/sc1",$("#f1").serialize(),function(){ 
                $("#f1").hide();
                var reload5sec=document.createElement('script');
                reload5sec.text='var count=4;var cdi=setInterval(function(){$("#cd").text(count);if(!count){clearInterval(cdi);location.reload();}count--;},1000);';
                $('#r1').html('<h3><b>Configuration saved <span style="color: green;">successfully</span>.</b></h3>This page will be reloaded in <span id="cd">5</span>sec.').append(reload5sec);
            }).fail(function(){
                $('#r1').html('<h3><b>Configuration <span style="color: red;">error</span>.</b></h3>');
            });
            event.preventDefault();
        });
'@
        ;
        HTMLScriptInReady=@'

        $.getJSON("/gc1", function(GC1){

            for(i=0;i<GC1.nm;i++){
                $("#buses").append('<div id="b'+i+'"><div class="pure-control-group"><label for="b'+i+'">bus'+i+' Input/Output Pins</label><input type="number" id="b'+i+'i" name="b'+i+'i" min="0" max="16" size="2"> / <input type="number" id="b'+i+'o" name="b'+i+'o" min="0" max="16" size="2"></div></div>');
            }

            $('#n').val(GC1.n);
            $("#n").prop("max",GC1.nm);
            $("#n").trigger("change");
    
            $.each(GC1,function(k,v){
                if(k.substring(0,1)=="b") $('#'+k).val(v);
            });

            if(GC1.e==1){
                $("#n").prop("disabled",true);
                $("#b0i").prop("disabled",true);
                $("#b0o").prop("disabled",true);
                $("#e").append("<b> ESP-01 have only this bus</b>");
            }

            $("input[type=submit]",$("#f1")).prop("disabled",false);
            $("#l1").fadeOut();
        })
        .fail(function(){
            $("#l1").html('<h6 style="display:inline;color:red;"><b> Failed</b></h6>');
        });
'@
    }
    ;
    #---fw.html---
    "fw.html"=@{
        ApplicationName=$applicationName
        ;
        ShortApplicationName=$shortApplicationName
    }
    ;
    #---discover.html---
    "discover.html"=@{
        ApplicationName=$applicationName
        ;
        ShortApplicationName=$shortApplicationName
    }
}

#call script that prepare Common Web Files and contain compression/Convert/Merge functions
. ..\src\data\_prepareCommonWebFiles.ps1

$path=(Split-Path -Path $MyInvocation.MyCommand.Path)
$templatePath=($path+"\..\src\data")

Write-Host "--- Prepare Application Web Files ---"
Convert-TemplatesWithCustoToCppHeader -templatePath $templatePath -filesAndCusto $templatesWithCustoFiles -destinationPath $path
Convert-FilesToCppHeader -Path $path -FileNames $specificFiles
Write-Host ""