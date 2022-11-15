'use strict';

(function (env) {
	/**
	 * Events -> onCreated(npc, args...)
	 */
	env.setCallBack("npc.created", function (npc, ...args) {
		try {
			// Set whether the npc supports these events
			env.setNpcEvents(npc,
				(npc.onCreated && 1 << 0) |
				(npc.onTimeout && 1 << 1) |
				(npc.onPlayerChats && 1 << 2) |
				(npc.onPlayerEnters && 1 << 3) |
				(npc.onPlayerLeaves && 1 << 4) |
				(npc.onPlayerTouchsMe && 1 << 5) |
				(npc.onPlayerLogin && 1 << 6) |
				(npc.onPlayerLogout && 1 << 7) |
				(npc.onNpcWarped && 1 << 8)
			);

			if (npc.onCreated)
				npc.onCreated.apply(npc, args);
		} catch (e) {
			env.reportException("NPC Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
		}
	});

	/**
	 * Event -> onPlayerChats(npc, player, message)
	 */
	env.setCallBack("npc.playerchats", function (npc, player, message) {
		try {
			if (npc.onPlayerChats)
				npc.onPlayerChats(player, message);
		} catch (e) {
			env.reportException("NPC Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
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
			env.reportException("NPC Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
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
			env.reportException("NPC Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
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
			env.reportException("NPC Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
		}
	});

	/**
	 * Event -> onPlayerLogin(npc, player)
	 */
	env.setCallBack("npc.playerlogin", function (npc, player) {
		try {
			if (npc.onPlayerLogin)
				npc.onPlayerLogin(player);
		} catch (e) {
			env.reportException(npc.name + " Exception at onPlayerLogin: " + e.name + " - " + e.message);
		}
	});

	/**
	 * Event -> onPlayerLogout(npc, player)
	 */
	env.setCallBack("npc.playerlogout", function (npc, player) {
		try {
			if (npc.onPlayerLogout)
				npc.onPlayerLogout(player);
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
			if (npc.onNpcWarped)
				npc.onNpcWarped.apply(npc, args);
		} catch (e) {
			env.reportException("NPC Warped Exception at " + npc.levelname + "," + npc.x + "," + npc.y + ": " + e.name + " - " + e.message);
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
