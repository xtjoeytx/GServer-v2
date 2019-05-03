'use strict';

/*
var callbacks = (function() {
  console.log("Start: ", this)
  function testFunc() {
    console.log("testFunc(): ", this);
    testFunc2();
  }

  function testFunc2() {
    console.log("testFunc2(): ", this);
  }
  return {
    "testFunc": testFunc,
    "testFunc2": testFunc2
  };
})();

callbacks["testFunc"]();
*/

(function (env) {
    print("Test Environment ", env);
    print("Test Environment Type: ", typeof env);
	
	/**
	 * Events -> onCreated(npc, args...)
	 */
    env.setCallBack("npc.created", function (npc, ...args) {
        try {
			// Set whether the npc supports these events
			env.setNpcEvents(npc,
				(npc.onCreated && 1 << 0) |
				(npc.onTimeout && 1 << 1) |
				(npc.onPlayerEnters && 1 << 2) |
				(npc.onPlayerLeaves && 1 << 3) |
				(npc.onPlayerTouchsMe && 1 << 4)
			);

            if (npc.onCreated)
                npc.onCreated.apply(npc, args);
        } catch (e) {
            print("Exception thrown in onCreated by NPC: ", e);
        }
    });

	/**
	 * Event -> onPlayerEnters(npc, player)
	 */
    env.setCallBack("npc.playerenters", function (npc, player) {
        try {
            if (npc.onPlayerEnters)
                npc.onPlayerEnters(player);
        } catch (e) {
            print("Exception thrown in onPlayerEnters by NPC: ", e);
        }
	});
	
	/**
	 * Event -> onPlayerLeaves(npc, player)
	 */
    env.setCallBack("npc.playerleaves", function (npc, player) {
        try {
            if (npc.onPlayerLeaves)
                npc.onPlayerLeaves(player);
        } catch (e) {
            print("Exception thrown in onPlayerLeaves by NPC: ", e);
        }
	});
	
	
	/**
	 * Event -> onPlayerTouchsMe(npc, player)
	 */
    env.setCallBack("npc.playertouchsme", function (npc, player) {
        try {
            if (npc.onPlayerTouchsMe)
                npc.onPlayerTouchsMe(player);
        } catch (e) {
            print("Exception thrown in onPlayerLeaves by NPC: ", e);
        }
	});

    /**
	 * Event -> onTimeout(npc, args...)
	 */
    env.setCallBack("npc.timeout", function (npc, ...args) {
        try {
            if (npc.onTimeout)
                npc.onTimeout.apply(npc, args);
        } catch (e) {
            print("Exception thrown in onTmeout by NPC: ", e);
        }
    });

    /*
     * Event -> Triggeractions
     */
    env.setCallBack("npc.trigger", function (npc, func, data) {
        print("NPC TRIGGER ACTION RECEIVED\n");
        print("NPC: " + npc);
        print("FUNCTION: " + func);
        print("DATA: " + data);

        try {
            func.call(npc, data);
        } catch (e) {
            print("Exception thrown in onTmeout by NPC: ", e);
        }
    });
});
