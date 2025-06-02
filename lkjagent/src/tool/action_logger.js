"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || (function () {
    var ownKeys = function(o) {
        ownKeys = Object.getOwnPropertyNames || function (o) {
            var ar = [];
            for (var k in o) if (Object.prototype.hasOwnProperty.call(o, k)) ar[ar.length] = k;
            return ar;
        };
        return ownKeys(o);
    };
    return function (mod) {
        if (mod && mod.__esModule) return mod;
        var result = {};
        if (mod != null) for (var k = ownKeys(mod), i = 0; i < k.length; i++) if (k[i] !== "default") __createBinding(result, mod, k[i]);
        __setModuleDefault(result, mod);
        return result;
    };
})();
Object.defineProperty(exports, "__esModule", { value: true });
exports.load_action_log = load_action_log;
exports.save_action_log = save_action_log;
exports.log_action = log_action;
exports.trim_log = trim_log;
exports.get_recent_log_entries = get_recent_log_entries;
const fs = __importStar(require("fs-extra"));
const path = __importStar(require("path"));
const DATA_DIR = path.join(process.cwd(), 'data');
const LOG_FILE = path.join(DATA_DIR, 'log.json');
/**
 * Load action log from log.json
 */
async function load_action_log() {
    try {
        if (await fs.pathExists(LOG_FILE)) {
            return await fs.readJson(LOG_FILE);
        }
        else {
            return [];
        }
    }
    catch (error) {
        console.error('Error loading action log:', error);
        return [];
    }
}
/**
 * Save action log to log.json
 */
async function save_action_log(log) {
    try {
        await fs.ensureDir(DATA_DIR);
        await fs.writeJson(LOG_FILE, log, { spaces: 2 });
    }
    catch (error) {
        console.error('Error saving action log:', error);
        throw error;
    }
}
/**
 * Add a log entry to the action log
 */
async function log_action(action, result) {
    try {
        const log = await load_action_log();
        const log_entry = {
            timestamp: Date.now(),
            action_index: result.action_index,
            action,
            result,
            result_summary: generate_result_summary(action, result)
        };
        log.push(log_entry);
        await save_action_log(log);
    }
    catch (error) {
        console.error('Error logging action:', error);
        throw error;
    }
}
/**
 * Trim log to maximum number of entries
 */
async function trim_log(max_entries) {
    try {
        const log = await load_action_log();
        if (log.length > max_entries) {
            const trimmed_log = log.slice(-max_entries);
            await save_action_log(trimmed_log);
        }
    }
    catch (error) {
        console.error('Error trimming log:', error);
        throw error;
    }
}
/**
 * Generate a human-readable summary of the action result
 */
function generate_result_summary(action, result) {
    const action_desc = `${action.kind}${action.target_path ? ` ${action.target_path}` : ''}`;
    if (result.status === 'error') {
        return `${action_desc} failed: ${result.error}`;
    }
    switch (action.kind) {
        case 'set':
            return `${action_desc} completed successfully`;
        case 'get':
            return `${action_desc} retrieved data`;
        case 'rm':
            return `${action_desc} deleted successfully`;
        case 'mv':
            return `${action.source_path} moved to ${action.target_path}`;
        case 'ls':
            const count = result.result ? Object.keys(result.result).length : 0;
            return `${action_desc} listed ${count} items`;
        case 'search':
            const results = result.result ? Object.keys(result.result).length : 0;
            return `${action_desc} found ${results} matches`;
        default:
            return `${action_desc} completed`;
    }
}
/**
 * Get recent log entries
 */
async function get_recent_log_entries(count = 10) {
    try {
        const log = await load_action_log();
        return log.slice(-count);
    }
    catch (error) {
        console.error('Error getting recent log entries:', error);
        return [];
    }
}
