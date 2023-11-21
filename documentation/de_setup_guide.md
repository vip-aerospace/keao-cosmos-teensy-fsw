@page development_environment_setup Development Enviornment Setup Guide

@todo Add setup for code formatting on save, ruler, spellcheck

This is a guide on how to set up your Development Environment (DE) to compile, 
run, and debug the Teensy Flight Software (FSW).

@section step1 Step 1: Install VSCode
The FSW is written and compiled using 
[Visual Studio Code (VSCode)](https://code.visualstudio.com/). Download and 
install this editor.

Once installed, click the Extensions tab on the left navigation menu.

![The Extensions tab is the fifth tab down on the left.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step1.png?raw=true)

Search for the C/C++ extension and install it if not already installed.

![Searching for the C/C++ extension made by Microsoft.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step2.png?raw=true)

@section step2 Step 2: Install PlatformIO
The PlatformIO IDE is used for programming the Teensy. It is installed as an 
extension to VSCode. Search for the PlatformIO IDE extension and install it.

![Searching for the PlatformIO IDE extension made by PlatformIO.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step3.png?raw=true)

You should then see the shell install the IDE. If you do not see this, restart 
VSCode.

![The shell installing PlatformIO IDE.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step4.png?raw=true)

After installation is complete, **RESTART** VSCode. Pressing "Reload Now" is not
sufficient.

![PlatformIO IDE installation is complete.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step5.png?raw=true)

Verify PlatformIO IDE is installed. You should see a new icon in the left 
navigation menu.

![PlatformIO IDE icon in the left navigation menu.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step6.png?raw=true)

@section step3 Step 3: Build a test project
You can now verify that the PlatformIO IDE has been installed correctly by 
creating a test project. Begin by pressing the home button on the bottom 
navigation bar.

![The home button on the bottom navigation bar.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step7.png?raw=true)

Click on "New Project". If this page does not appear, restart VSCode. If the 
page still doesn't appear, retry the installation process.

![The New Project button on the PIO Home page.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step8.png?raw=true)

Fill out the Project Wizard as shown and click Finish. Wait for the IDE to 
create the project.

- The Name is agent_artemis
- The Board is Teensy 4.1
- The Framework is Arduino
- The Location is set to Use default location

![The Project Wizard dialog.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step9.png?raw=true)

If VSCode prompts you to trust the authors, click Yes.

![The trust authors dialog.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step10.png?raw=true)

You should now see the project open in your workspace. 

![The agent_artemis project open in the VSCode workspace.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step11.png?raw=true)

The main code is located in ```src/main.cpp```. Add the following code to this 
file:
```
/**
 * Blink
 *
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */
#include "Arduino.h"

// Set LED_BUILTIN if it is not defined by Arduino framework
// #define LED_BUILTIN 13

void setup()
{
  // initialize LED digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
  // turn the LED on (HIGH is the voltage level)
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("Hello World");

  // wait for a second
  delay(1000);

  // turn the LED off by making the voltage LOW
  digitalWrite(LED_BUILTIN, LOW);

   // wait for a second
  delay(1000);
}
```

Before building the project, ensure the environment is set to Teensy 4.1. If the
environment (as shown in the bottom navigation bar) says "Default", change it.

Click on Default (agent_artemis).

![The Default Environment.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step12.png?raw=true)

On the top of VSCode, a dropdown should have appeared. Click on env:teensy41

![The Environment Selection dropdown.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step13.png?raw=true)

Verify your environment has changed.

![The teensy41 Environment.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step14.png?raw=true)

Click the checkmark on the bottom navigation bar to build.

![The build button on the bottom navigation bar.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step15.png?raw=true)

Connect a Teensy 4.1 to your computer over USB. Click the right arrow icon on 
the bottom navigation bar to upload the project to your Teensy.

![The upload button on the bottom navigation bar.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step16.png?raw=true)

After a successful upload, you can monitor the Serial output by clicking the 
plug icon on the bottom navigation bar.

![The monitor button on the bottom navigation bar.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step17.png?raw=true)

@section step4 Step 4: Clone the FSW
Now that you've uploaded a test program, you are ready to work with the FSW. The
easiest way to do so is with [GitHub Desktop.](https://desktop.github.com/)

Install it and log in using your GitHub credentials. Go to 
[the FSW repo.](https://github.com/vip-aerospace/Artemis-Teensy-Flight-Software)
If you do not have access to this repo, verify that you have been added to 
[the VIP Aerospace Technologies organization](https://github.com/vip-aerospace).
If you are not, ask team leadership to be added to the organization.

Once you have access to the repo, press the green "Code" button, then press the 
"Open with GitHub Desktop" button.

![The clone menu on the vip-aerospace/Artemis-Teensy-Flight-Software repo.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step18.png?raw=true)

GitHub Desktop will open with the Clone a repository dialog. You can choose 
where you want to save the repository here. It is usually a good idea to save 
the repo close to your root directory. Choose a local path and press "Clone".

![The clone a repository dialog in GitHub Desktop.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step19.png?raw=true)

Open the repo's directory in the Command Prompt/Terminal, and enter the 
following:

@code{shell}
$ git submodule update --init --recursive
@endcode

This initializes the micro-cosmos submodule, which the FSW relies on.

@code{shell}
$ git submodule update --remote --merge
@endcode

This ensures you have the most recent version of the submodule.

@section step5 Step 5: Run the FSW
With the FSW repo cloned to your local machine, you are now ready to use it. 
Press the "Open in Visual Studio Code" in GitHub Desktop to open the FSW. 

Alternatively, you can open VSCode, Press File then Open Folder... Navigate to 
the directory where you have the repo cloned (named 
Artemis-Teensy-Flight-Software). Select it, and press "Select Folder". 

VSCode will now load the FSW. PlatformIO will also load.

To build and upload the code to the Teensy, you can use the individual buttons 
as described earlier in this guide. Alternatively, press the PlatformIO tab on 
the left navigation menu. The PlatformIO palette will open on the right, with 
your current environment selected.

Under teensy41 > General, you have multiple options. To build, upload, and 
monitor the FSW all at once, press the "Upload and Monitor" option.

![The Upload and Monitor option in the PlatformIO palette.](https://github.com/vip-aerospace/teensy-fsw-documentation/blob/main/documentation/step20.png?raw=true)

The FSW will be compiled, uploaded, and sent to the Teensy. A Serial Monitor 
will be started in the Terminal window at the bottom.

