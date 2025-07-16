# GS2Emu Execscripts

## What are they?

Execscripts are a feature that allows bespoke weapons to be added to a player to execute scripts.
The weapon script has replacement tokens that allow a customized version to be added to each client.

## How to enable

Enable the hack in the `serveroptions.txt` file:

    triggerhack_execscript = true

## How to use

Put weapon scripts in the `execscripts/` folder with the file name of `weaponname.txt`.
The weapon script should provide tokens for replacements:

    if (created) {
      setplayerprop #c,*PARM0;
      destroy;
    }

Tokens are given the format of `*PARM#`, where the `#` is the triggeraction parameter number.
For the above weapon (given the file name of `forcesay.txt`), you would use the following triggeraction:

    if (playerchats && startswith(/sayhi,#c)) {
      tokenize #c;
      triggeraction 0,0,gr.es_clear,;
      triggeraction 0,0,gr.es_set,Hi;
      triggeraction 0,0,gr.es,#t(1),forcesay;
    }
