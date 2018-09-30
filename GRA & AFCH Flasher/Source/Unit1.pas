unit Unit1;

interface

uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, ShellAPI, Vcl.StdCtrls, StrUtils,
  Vcl.Buttons, System.Notification, AnsiStrings, Vcl.ExtCtrls;

type
  TGRA_AND_AFCH_FLASHER = class(TForm)
    Button1: TButton;
    ComboBox: TComboBox;
    BitBtn1: TBitBtn;
    Label1: TLabel;
    Edit1: TEdit;
    BitBtn2: TBitBtn;
    Label2: TLabel;
    Label3: TLabel;
    ComboBox1: TComboBox;
    OpenDialog1: TOpenDialog;
    Memo1: TMemo;
    Timer1: TTimer;
    procedure Button1Click(Sender: TObject);
    procedure BitBtn1Click(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure GetComPorts(aList: TStrings; aNameStart: string);
    procedure BitBtn2Click(Sender: TObject);
    function ExecAndCapture(const ACmdLine: string; var AOutput: string): Integer;
    procedure Timer1Timer(Sender: TObject);
  private
    { Private declarations }
  public
      { Public declarations }
  end;
type
  TAnoPipe=record
    Input : THandle;
    Output: THandle;
  end;

var
  GRA_AND_AFCH_FLASHER: TGRA_AND_AFCH_FLASHER;

implementation

{$R *.dfm}

function TGRA_AND_AFCH_FLASHER.ExecAndCapture(const ACmdLine: string; var AOutput: string): Integer;
const
  cBufferSize = 2048;
var
  vBuffer: Pointer;
  vStartupInfo: TStartUpInfo;
  vSecurityAttributes: TSecurityAttributes;
  vReadBytes: DWord;
  vProcessInfo: TProcessInformation;
  vStdInPipe : TAnoPipe;
  vStdOutPipe: TAnoPipe;
begin
  Result := 0;

  with vSecurityAttributes do
  begin
    nlength := SizeOf(TSecurityAttributes);
    binherithandle := True;
    lpsecuritydescriptor := nil;
  end;

  // Create anonymous pipe for standard input
  if not CreatePipe(vStdInPipe.Output, vStdInPipe.Input, @vSecurityAttributes, 0) then
    raise Exception.Create('Failed to create pipe for standard input. System error message: ' + SysErrorMessage(GetLastError));

  try
    // Create anonymous pipe for standard output (and also for standard error)
    if not CreatePipe(vStdOutPipe.Output, vStdOutPipe.Input, @vSecurityAttributes, 0) then
      raise Exception.Create('Failed to create pipe for standard output. System error message: ' + SysErrorMessage(GetLastError));

    try
      GetMem(vBuffer, cBufferSize);
      try
        // initialize the startup info to match our purpose
        FillChar(vStartupInfo, Sizeof(TStartUpInfo), #0);
        vStartupInfo.cb         := SizeOf(TStartUpInfo);
        //vStartupInfo.wShowWindow:= SW_HIDE;  // we don't want to show the process
        vStartupInfo.wShowWindow:= SW_SHOWNORMAL;
        // assign our pipe for the process' standard input
        vStartupInfo.hStdInput  := vStdInPipe.Output;
        // assign our pipe for the process' standard output
        vStartupInfo.hStdOutput := vStdOutPipe.Input;
        vStartupInfo.dwFlags    := STARTF_USESTDHANDLES or STARTF_USESHOWWINDOW;

        if not CreateProcess(nil
                             , PChar(ACmdLine)
                             , @vSecurityAttributes
                             , @vSecurityAttributes
                             , True
                             , NORMAL_PRIORITY_CLASS
                             , nil
                             , nil
                             , vStartupInfo
                             , vProcessInfo) then
        begin
          if (getlasterror=2) then raise Exception.Create('Failed creating the console process. File "run.cmd" not found.');//ShowMessage('');
          raise Exception.Create('Failed creating the console process. System error msg: ' + SysErrorMessage(GetLastError));
        end;
        try
          // wait until the console program terminated
          while WaitForSingleObject(vProcessInfo.hProcess, 5000)=WAIT_TIMEOUT do
            Sleep(0);

          // clear the output storage
          AOutput := '';
          // Read text returned by the console program in its StdOut channel
          repeat
            ReadFile(vStdOutPipe.Output, vBuffer^, cBufferSize, vReadBytes, nil);
            //ReadFileEx()
            //ReadFileEx(vStdOutPipe.Output, vBuffer, cBufferSize, nil, nil);
            if vReadBytes > 0 then
            begin
              //Memo1.Lines.Text := Trim(AnsiReplaceStr(vBuffer, #13#13#10, #13#10));
              Memo1.Lines.Add(AnsiStrings.StrPas(vBuffer));// := Trim(AnsiReplaceStr(vBuffer, #13#13#10, #13#10));
              Application.ProcessMessages();
              AOutput := AOutput + AnsiStrings.StrPas(vBuffer);
              Inc(Result, vReadBytes);
            end;
          until (vReadBytes < cBufferSize);
        finally
          CloseHandle(vProcessInfo.hProcess);
          CloseHandle(vProcessInfo.hThread);
        end;
      finally
        FreeMem(vBuffer);
      end;
    finally
      CloseHandle(vStdOutPipe.Input);
      CloseHandle(vStdOutPipe.Output);
    end;
  finally
    CloseHandle(vStdInPipe.Input);
    CloseHandle(vStdInPipe.Output);
  end;
end;

function GetNextSubstring(aBuf: string; var aStartPos: integer): string;
var
  vLastPos: integer;
begin
  if (aStartPos < 1) then
    begin
      raise ERangeError.Create('aStartPos must be greate then 0');
    end;

  if (aStartPos > Length(aBuf) ) then
    begin
      Result := '';
      Exit;
    end;

  vLastPos := PosEx(#0, aBuf, aStartPos);
  Result := Copy(aBuf, aStartPos, vLastPos - aStartPos);
  aStartPos := aStartPos + (vLastPos - aStartPos) + 1;
end;

procedure TGRA_AND_AFCH_FLASHER.BitBtn2Click(Sender: TObject);
begin
if (OpenDialog1.Execute()) then  Edit1.Text:=OpenDialog1.FileName;
end;

procedure TGRA_AND_AFCH_FLASHER.Button1Click(Sender: TObject);
var
CPU: String;
Params: String;
vOutput: string;
begin
  case(ComboBox1.ItemIndex) of
  0,1,2,3: CPU:='atmega328p';
  4:CPU:='atmega2560';
  end;
  if (AnsiUpperCase(ExtractFileExt(OpenDialog1.FileName))<>'.HEX') then
  //ShowMessage(AnsiUpperCase(ExtractFileExt(OpenDialog1.FileName)));
  begin
    ShowMessage('Select *.HEX file');
    exit;
  end;
  //Memo1.Lines.Clear;
  //Memo1.Lines.Add(OpenDialog1.FileName);
  Params:='-Cavrdude.conf -v -V -p'+CPU+' -carduino -P'+ComboBox.Text+' -b115200 -D -Uflash:w:"'+OpenDialog1.FileName+'":i';
  //if (ShellExecute(Handle, nil, 'avrdude.exe', PChar(Params), nil, SW_SHOW)<=32) then
  //ExecAndCapture(PChar('avrdude.exe '+Params), vOutput);
  //Params:='-Cavrdude.conf -v -V -p'+CPU+' -carduino -P'+ComboBox.Text+' -b115200 -D -Uflash:w:'+OpenDialog1.FileName+':a';
  ExecAndCapture(PChar('run.cmd '+ Params),vOutput);
  //Showmessage(inttostr(GetLastError));
  if (GetLastError=2) then ShowMessage('Error: file "run.cmd" not found!');
  Memo1.Lines.Text := Trim(AnsiReplaceStr(vOutput, #13#13#10, #13#10));
end;

procedure TGRA_AND_AFCH_FLASHER.FormCreate(Sender: TObject);
begin
GetComPorts(ComboBox.Items, 'COM');
//ComboBox.Text:=ComboBox.Items[0];
ComboBox.ItemIndex:=0;
end;

procedure TGRA_AND_AFCH_FLASHER.GetComPorts(aList: TStrings; aNameStart: string);
var
  vBuf: string;
  vRes: integer;
  vErr: Integer;
  vBufSize: Integer;
  vNameStartPos: Integer;
  vName: string;
begin
  vBufSize := 1024 * 5;
  vRes := 0;

  while vRes = 0 do
    begin
      setlength(vBuf, vBufSize) ;
      SetLastError(ERROR_SUCCESS);
      vRes := QueryDosDevice(nil, @vBuf[1], vBufSize) ;
      vErr := GetLastError();

      //¬ариант дл¤ двухтонки
      if (vRes <> 0) and (vErr = ERROR_INSUFFICIENT_BUFFER) then
        begin
          vBufSize := vRes;
          vRes := 0;
        end;

      if (vRes = 0) and (vErr = ERROR_INSUFFICIENT_BUFFER) then
        begin
          vBufSize := vBufSize + 1024;
        end;

      if (vErr <> ERROR_SUCCESS) and (vErr <> ERROR_INSUFFICIENT_BUFFER) then
        begin
          raise Exception.Create(SysErrorMessage(vErr) );
        end
    end;
  setlength(vBuf, vRes) ;

  vNameStartPos := 1;
  vName := GetNextSubstring(vBuf, vNameStartPos);

  aList.BeginUpdate();
  try
    aList.Clear();
    while vName <> '' do
      begin
        if StartsStr(aNameStart, vName) then
          aList.Add(vName);
        vName := GetNextSubstring(vBuf, vNameStartPos);
      end;
  finally
    aList.EndUpdate();
  end;
end;

procedure TGRA_AND_AFCH_FLASHER.Timer1Timer(Sender: TObject);
var
curItem: Integer;
begin
//  GetComPorts(ComboBox.Items, 'COM');
curItem:=ComboBox.ItemIndex;
GetComPorts(ComboBox.Items, 'COM');
//BitBtn1Click(nil);
ComboBox.ItemIndex:=curItem;
end;

procedure TGRA_AND_AFCH_FLASHER.BitBtn1Click(Sender: TObject);
begin
  GetComPorts(ComboBox.Items, 'COM');
  ComboBox.ItemIndex:=0;
end;

end.
