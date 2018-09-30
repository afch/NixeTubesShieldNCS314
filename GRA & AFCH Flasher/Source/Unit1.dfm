object GRA_AND_AFCH_FLASHER: TGRA_AND_AFCH_FLASHER
  Left = 0
  Top = 0
  Caption = 'GRA & AFCH Nixie Tubes Clocks Firmware Flasher v1.2'
  ClientHeight = 339
  ClientWidth = 457
  Color = clBtnFace
  Constraints.MinHeight = 374
  Constraints.MinWidth = 473
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 8
    Top = 11
    Width = 87
    Height = 13
    Caption = 'Select the port:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
  end
  object Label2: TLabel
    Left = 8
    Top = 51
    Width = 82
    Height = 13
    Caption = 'Select .HEX file'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
  end
  object Label3: TLabel
    Left = 8
    Top = 88
    Width = 79
    Height = 13
    Caption = 'Select Device:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'Tahoma'
    Font.Style = [fsBold]
    ParentFont = False
  end
  object Button1: TButton
    Left = 152
    Top = 112
    Width = 161
    Height = 57
    Align = alCustom
    Anchors = [akLeft, akTop, akRight]
    Caption = 'FLASH!'
    TabOrder = 0
    OnClick = Button1Click
  end
  object ComboBox: TComboBox
    Left = 101
    Top = 8
    Width = 156
    Height = 21
    Align = alCustom
    Style = csDropDownList
    Anchors = [akLeft, akTop, akRight]
    Sorted = True
    TabOrder = 1
  end
  object BitBtn1: TBitBtn
    Left = 263
    Top = 6
    Width = 186
    Height = 25
    Align = alCustom
    Anchors = [akTop, akRight]
    Caption = 'Search for available ports'
    TabOrder = 2
    OnClick = BitBtn1Click
  end
  object Edit1: TEdit
    Left = 101
    Top = 48
    Width = 294
    Height = 21
    Align = alCustom
    Anchors = [akLeft, akTop, akRight]
    TabOrder = 3
  end
  object BitBtn2: TBitBtn
    Left = 401
    Top = 48
    Width = 48
    Height = 22
    Align = alCustom
    Anchors = [akTop, akRight]
    Caption = 'Open'
    TabOrder = 4
    OnClick = BitBtn2Click
  end
  object ComboBox1: TComboBox
    Left = 101
    Top = 85
    Width = 348
    Height = 21
    Align = alCustom
    Style = csDropDownList
    Anchors = [akLeft, akTop, akRight]
    ItemIndex = 2
    TabOrder = 5
    Text = 'MCU 109 (Atmega328p)'
    Items.Strings = (
      'MCU 105 (Atmega328p)'
      'MCU 107 (Atmega328p)'
      'MCU 109 (Atmega328p)'
      'Arduino Shield NCS314 / NCS312 - Arduino UNO (Atmega328p)'
      'Arduino Shield NCS314 / NCS312 - Arduino Mega 2560 (Atmega2560)')
  end
  object Memo1: TMemo
    Left = 0
    Top = 189
    Width = 457
    Height = 150
    Align = alCustom
    Anchors = [akLeft, akTop, akRight, akBottom]
    ScrollBars = ssBoth
    TabOrder = 6
  end
  object OpenDialog1: TOpenDialog
    DefaultExt = '.hex'
    Filter = 'Arduinno compiled sketch|*.hex'
    Left = 72
    Top = 128
  end
  object Timer1: TTimer
    OnTimer = Timer1Timer
    Left = 376
    Top = 136
  end
end
