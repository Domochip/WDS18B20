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
        <h2 class="content-subhead">OneWire <span id="l2"><h6 style="display:inline"><b> Loading...</b></h6></span></h2>
        <div id="AllSensors"></div>
'@
        ;
        HTMLScriptInReady=@'
            $.getJSON("/gc", function(GC){

                var buses = [];
                for(i=0;i<GC.n;i++) buses.push(i);

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
            });
            $("#l2").fadeOut();
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
        <h2 class="content-subhead">OneWire</h2>
        <div class="pure-control-group">
            <label for="n">number Of OW Bus</label>
            <input type='number' id='n' name='n' min='1'><span id='e'></span>
        </div>
        <div id="buses"></div>
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
'@
        ;
        HTMLFillinConfigForm=@'
        for(i=0;i<GC.nm;i++){
            $("#buses").append('<div id="b'+i+'"><div class="pure-control-group"><label for="b'+i+'">bus'+i+' Input/Output Pins</label><input type="number" id="b'+i+'i" name="b'+i+'i" min="0" max="16" size="2"> / <input type="number" id="b'+i+'o" name="b'+i+'o" min="0" max="16" size="2"></div></div>');
        }

        $("#n").prop("max",GC.nm);
        $("#n").trigger("change");
        $.each(GC,function(k,v){
            if(k.substring(0,1)=="b") $('#'+k).val(v);
        });

        if(GC.e==1){
            $("#n").prop("disabled",true);
            $("#b0i").prop("disabled",true);
            $("#b0o").prop("disabled",true);
            $("#e").append("<b> ESP-01 have only this bus</b>");
        }
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