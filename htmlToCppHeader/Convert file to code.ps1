#--------------------------------------------------------------------------------------

#--------------V 2.0 --------------


function Remove-StringLatinCharacters
{
    PARAM ([string]$String)
    [Text.Encoding]::ASCII.GetString([Text.Encoding]::GetEncoding("Cyrillic").GetBytes($String))
}
function BinaryToCppHeader($binaryPath)
{

    if(Test-Path ($binaryPath+".h")) {Remove-Item ($binaryPath+".h");}
    
    #open files
    [System.IO.FileStream] $binaryFile = [System.IO.File]::OpenRead($pwd.Path+"\"+$binaryPath);
    [System.IO.FileStream] $cppFile = [System.IO.File]::OpenWrite($pwd.Path+"\"+$binaryPath+".h");

    #Prepare start of code and write it
    $text+="const PROGMEM char "+ (Remove-StringLatinCharacters (Split-Path -Path $binaryPath -Leaf).Replace(' ','').Replace('.','').Replace('-','')) + "[] = {";
    $cppFile.Write([System.Text.UTF8Encoding]::new().GetBytes($text),0,$text.Length);


    $first=$true;

    while($binaryFile.Position -ne $binaryFile.Length){

        #
        $text = if($first){""}else{","};
        $first=$false;
        $text+= "0x"+[System.BitConverter]::ToString($binaryFile.ReadByte());
        $cppFile.Write([System.Text.UTF8Encoding]::new().GetBytes($text),0,$text.Length);
    }

    $text="};";
    $cppFile.Write([System.Text.UTF8Encoding]::new().GetBytes($text),0,$text.Length);

    $binaryFile.Close();
    $cppFile.Close();
}


Import-Module -Name ".\7Zip4Powershell\1.8.0\7Zip4PowerShell"

$path="..\data"

$file="fw.html"
Compress-7Zip -Path $path -Filter $file -ArchiveFileName ($path+"\"+$file+".gz") -CompressionLevel Ultra
BinaryToCppHeader($path+"\"+$file+".gz")
Remove-Item ($path+"\"+$file+".gz")

$file="jquery-3.1.1.min.js"
Compress-7Zip -Path $path -Filter $file -ArchiveFileName ($path+"\"+$file+".gz") -CompressionLevel Ultra
BinaryToCppHeader($path+"\"+$file+".gz")
Remove-Item ($path+"\"+$file+".gz")

$file="config.html"
Compress-7Zip -Path $path -Filter $file -ArchiveFileName ($path+"\"+$file+".gz") -CompressionLevel Ultra
BinaryToCppHeader($path+"\"+$file+".gz")
Remove-Item ($path+"\"+$file+".gz")

$file="status.html"
Compress-7Zip -Path $path -Filter $file -ArchiveFileName ($path+"\"+$file+".gz") -CompressionLevel Ultra
BinaryToCppHeader($path+"\"+$file+".gz")
Remove-Item ($path+"\"+$file+".gz")

$file="test.html"
Compress-7Zip -Path $path -Filter $file -ArchiveFileName ($path+"\"+$file+".gz") -CompressionLevel Ultra
BinaryToCppHeader($path+"\"+$file+".gz")
Remove-Item ($path+"\"+$file+".gz")