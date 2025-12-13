// TODO:
// - товстіші стійки для плати реле
// - товтіші стійки для контоллера
// - стійки сенсора підняти вгору
// - друкувати кришку суцільним шаром
// - можливо збільшити розмір отвору для реле

// 1. СПОЧАТКУ оголошуємо ВСІ власні змінні
relayHeight = 18;
AcdcHeight = 17.0;

capAboveLid = 2; //-- How much the button cap extends above the lid when not pressed
switchHeight = 4.3; //-- Height of the switch body below the lid

// 2. ПОТІМ включаємо YAPPgenerator
include <./YAPPgenerator_v3.scad>
//-- Edit these parameters for your own box
wallThickness = 2.5;
basePlaneThickness = 2.0;
lidPlaneThickness = 1.2;

lidWallHeight = 7;
baseWallHeight = 12;

showPCB = true;

//-- which part(s) do you want to print?
renderQuality = 12; //-> from 1 to 32, Default = 8
printBaseShell = true;
printLidShell = true;
printSwitchExtenders = true;

// --Preview --
previewQuality = 5; //-> from 1 to 32, Default = 5
// showSideBySide = false; //-> Default = true
onLidGap = 0; //-- tip don't override to animate the lid opening
colorLid = "YellowGreen";
alphaLid = 1;
colorBase = "BurlyWood";
alphaBase = 1;

hideLidWalls = false; //-- Remove the walls from the lid : only if preview and showSideBySide=true 
hideBaseWalls = false; //-- Remove the walls from the base : only if preview and showSideBySide=true  
showOrientation = true; //-- Show the Front/Back/Left/Right labels : only in preview
showSwitches = true; //-- Show the switches (for pushbuttons) : only in preview 
// showButtonsDepressed = true; //-- Should the buttons in the Lid On view be in the pressed position
showOriginCoordBox = false; //-- Shows red bars representing the origin for yappCoordBox : only in preview 
showOriginCoordBoxInside = false; //-- Shows blue bars representing the origin for yappCoordBoxInside : only in preview 
showOriginCoordPCB = false; //-- Shows blue bars representing the origin for yappCoordBoxInside : only in preview 
// showMarkersPCB = true; //-- Shows black bars corners of the PCB : only in preview 
showMarkersCenter = false; //-- Shows magenta bars along the centers of all faces  
// inspectX = 10; //-> 0=none (>0 from Back)
// inspectY = 49; //-> 0=none (>0 from Right)
// inspectZ = 5; //-> 0=none (>0 from Bottom)
inspectXfromBack = true; //-> View from the inspection cut foreward
inspectYfromLeft = true; //-> View from the inspection cut to the right
inspectZfromBottom = true; //-> View from the inspection cut up
inspectButtons = 1; //-- Show the buttons in the inspection cut : only in preview

pcb =
[
  //-- Default Main PCB - DO NOT REMOVE the "Main" line.
  // p(0)name, p(1)length, p(2)width, p(3)posx, p(4)posy, p(5)Thickness, p(6)standoff_Height, p(7)standoff_Diameter,
  // p(8)standoff_PinDiameter, p(9)standoff_HoleSlack (default to 0.4)
  // standoffHeight, standoffDiameter, standoffPinDiameter, standoffHoleSlack
  ["sensor", 21.7, 17.7, 37, 30.2 + 10 + 15 + 25, 1.6, 3, 5, 0],
  [
    "b_pcb",
    32,
    23,
    0,
    34 + 18 + 25,
    1, //Thickness
    0,
  ],
  [
    "Main",
    55.5 + 2 * standoffPinDiameter / 2 + 2,
    25.8 + 2 * standoffPinDiameter / 2 + 2,
    0,
    18 + 26,
    1.6, // thickness
    13, // standoffHeight
    7, // standoffDiameter
    2.3, // standoffPinDiameter
    standoffHoleSlack,
  ],
  [
    "relay",
    50 + 2 + 1,
    15 + 1,
    0,
    24,
    1.5,
    2,
    5,
    1.7,
  ],
  ["power", 32, 22, 0, 0, AcdcHeight, 0],
];

pcbStands =
//    p(0)  = posX
//    p(1)  = posY
//   Optional:
//    p(2)  = Height to bottom of PCB : Default = standoffHeight
//    p(3)  = PCB Gap : Default = -1 : Default for yappCoordPCB=pcbThickness, yappCoordBox=0
//    p(4)  = standoffDiameter    Default = standoffDiameter;
//    p(5)  = standoffPinDiameter Default = standoffPinDiameter;
//    p(6)  = standoffHoleSlack   Default = standoffHoleSlack;
//    p(7)  = filletRadius (0 = auto size)
// p(8) = Pin Length : Default = 0 -> PCB Gap + standoff_PinDiameter
[
  [
    2,
    2,
    default,
    default,
    default,
    default,
    default,
    default,
    2,
    yappAllCorners,
  ],
  [standoffPinDiameter - 1, standoffPinDiameter - 1, yappAllCorners, [yappPCBName, "sensor"]], //-- Add pcbStands 5mm for all four corners of the PCB
  [1.5, 1.5, default, default, default, default, yappAllCorners, [yappPCBName, "relay"]],
];

//-- padding between pcb and inside wall
paddingFront = 6;
paddingBack = 2;
paddingRight = 2;
paddingLeft = 2;

snapJoins =
[
  [10, 5, yappLeft, yappRight, yappSymmetric],
  [15, 5, yappFront, yappBack, yappSymmetric],
];

boxMounts =
[
  [7, 3.5, 5, 3, yappBack],
  [shellWidth - 12, 3.5, 5, 3, yappBack],
  [7, 3.5, 5, 3, yappFront],
  [shellWidth - 12, 3.5, 5, 3, yappFront],
];

pushButtons =
//  Parameters:
//   Required:
//    p(0) = posx
//    p(1) = posy
//    p(2) = capLength 
//    p(3) = capWidth 
//    p(4) = capRadius 
//    p(5) = capAboveLid
//    p(6) = switchHeight
//    p(7) = switchTravel
//    p(8) = poleDiameter
//   Optional:
//    p(9) = Height to top of PCB : Default = standoffHeight + pcbThickness
//    p(10) = { yappRectangle | yappCircle | yappPolygon | yappRoundedRect 
//                    | yappCircleWithFlats | yappCircleWithKey } : Shape, Default = yappRectangle
//    p(11) = angle : Default = 0
//    p(12) = filletRadius          : Default = 0/Auto 
//    p(13) = buttonWall            : Default = 2.0;
//    p(14) = buttonPlateThickness  : Default= 2.5;
//    p(15) = buttonSlack           : Default= 0.25;
//    p(16) = snapSlack             : Default= 0.10;
[
  [
    7,
    8,
    0,
    0,
    4,
    capAboveLid,
    switchHeight,
    0.5,
    3,
    default, //Height to top of PCB : Default = standoffHeight + pcbThickness
    yappCircle,
    0,
    4,
    4,
    1.5,
    0.15,
    0.1,
    [yappPCBName, "b_pcb"],
  ],
  [
    20,
    17,
    0,
    0,
    4,
    capAboveLid,
    switchHeight,
    0.5,
    3,
    default, //Height to top of PCB : Default = standoffHeight + pcbThickness
    yappCircle,
    0,
    4,
    3.5,
    1.5,
    0.15,
    0.1,
    [yappPCBName, "b_pcb"],
  ],
];

//  Required:
//     p(0) = from Back
//     p(1) = from Left
//     p(2) = width
//     p(3) = length
//     p(4) = radius
//     p(5) = shape : { yappRectangle | yappCircle | yappPolygon | yappRoundedRect 
//                     | yappCircleWithFlats | yappCircleWithKey }
//   Optional:
//     p(6) = depth : Default = 0/Auto : 0 = Auto (plane thickness)
//     p(7) = angle : Default = 0

cutoutsLid =
[
  [27, 10, 24, 15, 0, yappRectangle], //-- A
  [7, 7, 10, 10, 5, yappCircle, [maskSquares, 2, 3, 5], [yappPCBName, "sensor"]],
  [5, 5, 30, 10, 0, yappRectangle, [maskOffsetBars, 0, 0, 0], [yappPCBName, "power"]],
];

cutoutsBase =
[
  [10, 5, 30, 10, 0, yappRectangle, [maskOffsetBars, 0, 0, 0], [yappPCBName, "power"]],
];

cutoutsFront =
[
  [5, 5, 8, 7, 0, yappRectangle, yappDefault, 0, yappCenter, [yappPCBName, "relay"]],
];

YAPPgenerate();
