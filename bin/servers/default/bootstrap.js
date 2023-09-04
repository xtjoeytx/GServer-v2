'use strict';

const INTERNAL_VAR = "__internal";

(function (env) {
	function addInternalData(obj, key, value) {
		if (!(INTERNAL_VAR in obj)) {
			obj[INTERNAL_VAR] = {
				'': obj['__main'] || {}
			};
		}

		if (typeof value !== 'object') {
			delete obj[INTERNAL_VAR][key];
		} else {
			obj[INTERNAL_VAR][key] = value;
		}

		mapInternalData(obj);
	}

	function unmapInternalData(obj) {
		if ("__public" in obj) {
			for (const fnName in obj['__public'])
				obj[fnName] = undefined;
		}

		obj['__events'] = {};
		obj['__public'] = {};
	}

	function mapInternalData(obj) {
		if (!(INTERNAL_VAR in obj)) {
			return;
		}

		unmapInternalData(obj);

		const internalData = obj[INTERNAL_VAR];
		for (const [className, classData] of Object.entries(internalData)) {
			const scopeFn = internalData[className]['scope'];

			for (const [ dataType, dataRows] of Object.entries(classData)) {
				if (dataType === "events" && typeof scopeFn === 'function') {
					for (const [ fnName, fnObj ] of Object.entries(dataRows)) {
						if (!(fnName in obj["__events"]))
							obj["__events"][fnName] = [];
						obj["__events"][fnName].push([fnObj, scopeFn]);
					}
				} else if (dataType === "public") {
					for (const [ fnName, fnObj ] of Object.entries(dataRows)) {
						obj["__public"][fnName] = true;
						obj[fnName] = fnObj;
					}
				}
			}
		}
	}

	function callEvent(obj, player, eventName, ...args) {
		if (eventName in obj['__events']) {
			for (const val of obj['__events'][eventName]) {
				// val[0] -> event function
				// val[1] -> set player scope
				val[1](player);
				val[0].apply(obj, args);
				val[1](undefined);
			}
		}
	}

	/**
	 * NPC -> Join Class
	 */
	env.setCallBack("npc.joinclass", function (npc, className, classData) {
		try {
			if (className.length > 0) {
				addInternalData(npc, className, classData);
				env.setNpcEvents(npc, Object.keys(npc["__events"] || {}));
			}
		} catch (e) {
			env.reportException("NPC Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
		}
	});

	/**
	 * NPC -> Leave Class
	 */
	env.setCallBack("npc.leaveclass", function (npc, className) {
		try {
			if (className.length > 0) {
				addInternalData(npc, className, undefined);
				env.setNpcEvents(npc, Object.keys(npc["__events"] || {}));
			}
		} catch (e) {
			env.reportException("NPC Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
		}
	});

	/**
	 * Events -> onCreated(npc, args...)
	 */
	env.setCallBack("npc.created", function (npc) {
		try {
			// Set whether the npc supports these events
			addInternalData(npc, "", npc.__main);
			env.setNpcEvents(npc, Object.keys(npc["__events"] || {}));

			callEvent(npc, undefined, "onCreated");
		} catch (e) {
			env.reportException("NPC Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
		}
	});

	// Eventually migrate everything to this
	env.setCallBack("npc.callevent", function (npc, player, eventName, args) {
		try {
			callEvent(npc, player, eventName, args);
		} catch (e) {
			env.reportException("NPC Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
		}
	});

	/**
	 * Event -> onPlayerChats(npc, player, message)
	 */
	env.setCallBack("npc.playerchats", function (npc, player, message) {
		try {
			callEvent(npc, player, "onPlayerChats", player, message);
		} catch (e) {
			env.reportException("NPC Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
		}
	});

	/**
	 * Event -> onPlayerEnters(npc, player)
	 */
	env.setCallBack("npc.playerenters", function (npc, player) {
		try {
			callEvent(npc, player, "onPlayerEnters", player);
		} catch (e) {
			env.reportException("NPC Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
		}
	});

	/**
	 * Event -> onPlayerLeaves(npc, player)
	 */
	env.setCallBack("npc.playerleaves", function (npc, player) {
		try {
			callEvent(npc, player, "onPlayerLeaves", player);
		} catch (e) {
			env.reportException("NPC Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
		}
	});

	/**
	 * Event -> onPlayerTouchsMe(npc, player)
	 */
	env.setCallBack("npc.playertouchsme", function (npc, player) {
		try {
			callEvent(npc, player, "onPlayerTouchsMe", player);
		} catch (e) {
			env.reportException("NPC Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
		}
	});

	/**
	 * Event -> onPlayerLogin(npc, player)
	 */
	env.setCallBack("npc.playerlogin", function (npc, player) {
		try {
			callEvent(npc, player, "onPlayerLogin", player);
		} catch (e) {
			env.reportException(npc.name + " Exception at onPlayerLogin: " + e.name + " - " + e.message);
		}
	});

	/**
	 * Event -> onPlayerLogout(npc, player)
	 */
	env.setCallBack("npc.playerlogout", function (npc, player) {
		try {
			callEvent(npc, player, "onPlayerLogout", player);
		} catch (e) {
			env.reportException(npc.name + " Exception at onPlayerLogout: " + e.name + " - " + e.message);
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
			env.reportException("NPC Timeout Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
		}
	});

	/**
	 * Event -> onNpcWarped(npc)
	 */
	env.setCallBack("npc.warped", function (npc, ...args) {
		try {
			callEvent(npc, player, "onNpcWarped", ...args);
		} catch (e) {
			env.reportException("NPC Warped Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
		}
	});

	env.setCallBack("npc.triggeron", function (npc, evName, player, data) {
		try {
			if (npc[evName]) {
				const params = tokenize(data, ',');
				npc[evName].apply(npc, params);
			} else {
				env.reportException("Unknown event: " + evName);
            }
		} catch (e) {
			env.reportException("NPC Trigger Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
		}
	});

	/*
	 * Event -> Triggeractions
	 */
	env.setCallBack("npc.trigger", function (npc, func, player, data) {
		try {
			const params = tokenize(data, ',');
			func.call(npc, player, params);
		} catch (e) {
			env.reportException("NPC Trigger Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
		}
	});

	/**
	 * Events -> weapon.onCreated(npc, args...)
	 */
	env.setCallBack("weapon.created", function (weapon, ...args) {
		if (weapon.onCreated)
			weapon.onCreated.apply(weapon, args);
	});

	/*
	 * Event -> weapon.onActionServerSide(player, data)
	 */
	env.setCallBack("weapon.serverside", function (weapon, player, data) {
		if (weapon.onActionServerSide) {
			const params = tokenize(data, ',');
			weapon.onActionServerSide(player, params);
		}
	});

	/*
	 * Global Function -> Tokenize
	 */
	env.global.tokenize = function(string, sep = ' ') {
		let separator = sep[0];
		let insideQuote = false;
		let stringList = [];
		let currentString = "";

		let stringLength = string.length;
		for (let i = 0; i < stringLength; i++) {
			switch (string[i]) {
				case separator: {
					if (!insideQuote) {
						stringList.push(currentString);
						currentString = "";
					}
					else currentString += string[i];

					break;
				}

				case '\"': {
					insideQuote = !insideQuote;
					break;
				}

				case '\\': {
					if (i + 1 < stringLength) {
						switch (string[i+1]) {
							case '"':
							case '\\':
								i++;

							default:
								currentString += string[i];
								break;
						}
					}
					else currentString += string[i];
					break;
				}

				default: {
					currentString += string[i];
					break;
				}
			}
		}

		stringList.push(currentString);
		return stringList;
	};

	// Math helper functions
	(function() {
		const _intVecX = [0, -1, 0, 1];
		const _intVecY = [-1, 0, 1, 0];

		env.global.vecx = function(dir) {
			return _intVecX[dir % 4];
		};

		env.global.vecy = function(dir) {
			return _intVecY[dir % 4];
		};

		env.global.random = function(min, max) {
			return Math.random() * (max - min) + min;
		};

		env.global.abs = function(num) {
			return Math.abs(num);
		};

		env.global.arctan = function (angle) {
			return Math.atan(angle);
		};

		env.global.char = function (code) {
			return String.fromCharCode(code);
		};

		env.global.cos = function (angle) {
			return Math.cos(angle);
		};

		env.global.exp = function (x) {
			return Math.exp(x);
		};

		env.global.float = function(num) {
			return parseFloat(num) || 0;
		};

		env.global.getangle = function (dx, dy) {
			return Math.atan2(dx, dy);
		};

		env.global.int = function(num) {
			return parseInt(num) || 0;
		};

		env.global.log = function (base, x) {
			return Math.log(x) / Math.log(base);
		};

		env.global.max = function(n1, n2) {
			return Math.max(n1, n2);
		};

		env.global.min = function(n1, n2) {
			return Math.min(n1, n2);
		};

		env.global.sin = function (angle) {
			return Math.sin(angle);
		};
	})();

	// Server functions
	(function() {
		env.global.createlevel = function(...args) {
			return server.createlevel(...args);
		};

		env.global.findlevel = function(...args) {
			return server.findlevel(...args);
		};

		env.global.findnpc = function(...args) {
			return server.findnpc(...args);
		};

		env.global.findplayer = function(...args) {
			return server.findplayer(...args);
		};

		env.global.savelog = function(...args) {
			return server.savelog(...args);
		};

		env.global.sendtorc = function(...args) {
			return server.sendtorc(...args);
		};

		env.global.sendtonc = function(...args) {
			return server.sendtonc(...args);
		};

		Object.defineProperty(env.global, 'allplayers', {
			get: function() {
				return server.players;
			}
		});

		Object.defineProperty(env.global, 'timevar', {
			get: function() {
				return server.timevar;
			}
		});

		Object.defineProperty(env.global, 'timevar2', {
			get: function() {
				return server.timevar2;
			}
		});
	})();
});
