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
        <h2 class="content-subhead">Home Automation<span id="l4"><h6 style="display:inline"><b> Loading...</b></h6></span></h2>
        Last HomeAutomation Result : <span id="lhar"></span><br>
'@
        ;
        HTMLScriptInReady=@'
            $.getJSON("/gs1", function(GS1){
                $.each(GS1,function(k,v){
                    if(k.length==1 && k.charAt(0)>='0' && k.charAt(0)<='9'){
                        $("#AllSensors").append("<h3>Bus "+k+"</h3></div>");
                        $("#AllSensors").append("<div id='bus"+k+"' class='pure-controls'></div>");
                        
                        $.each(v,function(romCode,temperature){
                            $("#bus"+k).append(romCode+" : <a class='pure-button pure-button-primary' href='/getT?bus"+k+"="+romCode+"'>"+temperature+"°C</a><br>");
                        });
                    }
                    else{
                        $('#'+k).html(v);
                    }
                });
                $("#l3").fadeOut();
                $("#l4").fadeOut();
            })
            .fail(function(){
                $("#l3").html('<h4 style="display:inline;color:red;"><b> Failed</b></h4>');
                $("#l4").html('<h4 style="display:inline;color:red;"><b> Failed</b></h4>');
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
        

            <h3 class="content-subhead">Home Automation  <span id="hai" name="hai" class="infotip">?</span></h3>
            <div class="infotipdiv"  id="haidiv" name="haidiv" style="display:none;">
                HTTP GET request can be send to this device : <br>
                <b>http://$ip$/GetL?bus0</b> : Get list of sensors on Bus 0 (JSON List)<br>
                <b>http://$ip$/GetT?bus0=$romcode$</b> : Get temperature from a sensor on bus 0 (JSON)<br>
            </div>
        
            <div class="pure-control-group">
                <label for="haproto">Type</label>
                <select id='haproto' name='haproto'>
                    <option value="0">Disabled</option>
                    <option value="1">MQTT</option>
                </select>
            </div>
    
            <div id='hae' style='display:none'>
    
                <div class="pure-control-group">
                    <label for="hatls">SSL/TLS</label>
                    <input type='checkbox' id='hatls' name='hatls'>
                </div>
                <div class="pure-control-group">
                    <label for="hahost">Hostname</label>
                    <input type='text' id='hahost' name='hahost' maxlength='64' pattern='[A-Za-z0-9-.]+' size='50' placeholder="IP or DNS Name">
                    <span class="pure-form-message-inline">(Hostname should match with certificate name if SSL/TLS is enabled)</span>
                </div>
                <div class="pure-control-group">
                    <label for="haupperiod">Upload Period</label>
                    <input type='number' id='haupperiod' name='haupperiod' min='15' max='65535' placeholder="(in seconds)">
                </div>
    
                <div id='hame'>
    
                    <div class="pure-control-group">
                        <label for="hamtype">Type</label>
                        <select id='hamtype' name='hamtype'>
                            <option value="0">Generic (Separated topic)</option>
                            <option value="1">Generic (Same level Topic)</option>
                        </select>
                        <span id="hamtype0i" name="hamtype0i" class="infotip">?</span>
                        <span id="hamtype1i" name="hamtype1i" class="infotip" style="display:none;">?</span>
                    </div>
                    <div class="pure-control-group" id="hamtype0div" name="hamtype0div" style="display:none;">
                        <label></label>
                        <div class="infotipdiv">
                            Published topics : <br>
                            <b>/$romcode$/temperature</b> : temperature of sensors<br>
                        </div>
                    </div>
                    <div class="pure-control-group" id="hamtype1div" name="hamtype1div" style="display:none;">
                        <label></label>
                        <div class="infotipdiv">
                            Published topics : <br>
                            <b>/$romcode$</b> : temperature of sensors<br>
                        </div>
                    </div>
    
                    <div class="pure-control-group">
                        <label for="hamport">Port</label>
                        <input type='number' id='hamport' name='hamport' min='1' max='65535'>
                    </div>
                    <div class="pure-control-group">
                        <label for="hamu">Username</label>
                        <input type='text' id='hamu' name='hamu' maxlength='64' placeholder="optional">
                    </div>
                    <div class="pure-control-group">
                        <label for="hamp">Password</label>
                        <input type='password' id='hamp' name='hamp' maxlength='64' placeholder="optional">
                    </div>
    
                    <div id='hamgbte'>
                        <div class="pure-control-group">
                            <label for="hamgbt">Base Topic</label>
                            <input type='text' id='hamgbt' name='hamgbt' maxlength='64'>
                            <span id="hamgbti" name="hamgbti" class="infotip">?</span>
                        </div>
    
                        <div class="pure-control-group" id="hamgbtidiv" name="hamgbtidiv" style="display:none;">
                            <label></label>
                            <div class="infotipdiv">
                                Base Topic placeholders : <br>
                                <b>$bus$</b> : Bus number for each sensors<br>
                                <b>$sn$</b> : Serial Number of this device<br>
                                <b>$mac$</b> : WiFi MAC address of this device<br>
                                <b>$model$</b> : Model of this device<br>
                                Ex : DomoChip/<b>$sn$</b> or <b>$model$</b>/<b>$mac$</b>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

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

        $("#hai").click(function(){$("#haidiv").slideToggle(300);});

        $("#haproto").change(function(){
            switch($("#haproto").val()){
                case "0":
                    $("#hae").hide();
                    break;
                case "1":
                    $("#hae").show();
                    $("#hame").show();
                    break;
            }
        });
        
        $("#hamtype").change(function(){
            switch($("#hamtype").val()){
                case "0":
                    $("#hamgbte").show();
                    $("#hamtype0i").show();
                    $("#hamtype1i").hide();
                    $("#hamtype1div").hide();
                    break;
                case "1":
                    $("#hamgbte").show();
                    $("#hamtype0i").hide();
                    $("#hamtype0div").hide();
                    $("#hamtype1i").show();
                    break;
            }
        });
        $("#hamtype0i").click(function(){$("#hamtype0div").slideToggle(300);});
        $("#hamtype1i").click(function(){$("#hamtype1div").slideToggle(300);});
        $("#hamgbti").click(function(){$("#hamgbtidiv").slideToggle(300);});

        $("#hatls").change(function(){
            if($("#hatls").prop("checked")){
                $("#hahtlse").show();
                $("#hamport").val(8883);
            }
            else{
                $("#hahtlse").hide();
                $("#hamport").val(1883);
            }
        });

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

                if($('#'+k).prop('type')!='checkbox') $('#'+k).val(v);
                else $('#'+k).prop("checked",v);

                $('#'+k).trigger("change");
            })

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
. ..\base\data\_prepareCommonWebFiles.ps1

$path=(Split-Path -Path $MyInvocation.MyCommand.Path)
$templatePath=($path+"\..\base\data")

Write-Host "--- Prepare Application Web Files ---"
Convert-TemplatesWithCustoToCppHeader -templatePath $templatePath -filesAndCusto $templatesWithCustoFiles -destinationPath $path
Convert-FilesToCppHeader -Path $path -FileNames $specificFiles
Write-Host ""