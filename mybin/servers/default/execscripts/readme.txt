Store your weapon scripts in here.  They be in plain text files as the name of the script.

As an example, forcesay.txt:
---------------
if (playerenters || created || timeout) {
  timeout = 0.05;
  setplayerprop #c,*PARM0;
  destroy;
}
---------------

In an execscript, the gserver will load the script and replace all the parameters with the given list.
First, the gr.es_clear triggeraction will clear the parameter list.
Then, the gr.es_set and gr.es_append triggeractions are used to assemble the parameter list.
Finally, the gr.es triggeraction initiates the script.

As an example, we are going to use the forcesay.txt script to force a player to say, "Hi."
---------------
if (playerchats && startswith(/sayhi,#c)) {
  tokenize #c;
  triggeraction 0,0,gr.es_clear,;
  triggeraction 0,0,gr.es_set,Hi;
  triggeraction 0,0,gr.es,#t(1),forcesay;
}
---------------

First, we cleared the parameter list.
Then, we set *PARM0 to Hi.
Next, we sent the script off to the targetted player, whose account is #t(1).

You can use *PARM0 through *PARM128 in your execscript.