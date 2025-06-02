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
exports.load_working_memory = load_working_memory;
exports.save_working_memory = save_working_memory;
exports.load_storage = load_storage;
exports.save_storage = save_storage;
exports.get_working_memory_size = get_working_memory_size;
exports.is_working_memory_oversized = is_working_memory_oversized;
exports.trim_working_memory = trim_working_memory;
const fs = __importStar(require("fs-extra"));
const path = __importStar(require("path"));
const DATA_DIR = path.join(process.cwd(), 'data');
const MEMORY_FILE = path.join(DATA_DIR, 'memory.json');
const STORAGE_FILE = path.join(DATA_DIR, 'storage.json');
/**
 * Load working memory from memory.json
 */
async function load_working_memory() {
    try {
        if (await fs.pathExists(MEMORY_FILE)) {
            const data = await fs.readJson(MEMORY_FILE);
            return data.working_memory;
        }
        else {
            // Return default structure
            return {
                user_data: { todo: {} },
                action_result: {},
                system_info: {}
            };
        }
    }
    catch (error) {
        console.error('Error loading working memory:', error);
        return {
            user_data: { todo: {} },
            action_result: {},
            system_info: {}
        };
    }
}
/**
 * Save working memory to memory.json
 */
async function save_working_memory(working_memory) {
    try {
        await fs.ensureDir(DATA_DIR);
        const data = { working_memory };
        await fs.writeJson(MEMORY_FILE, data, { spaces: 2 });
    }
    catch (error) {
        console.error('Error saving working memory:', error);
        throw error;
    }
}
/**
 * Load storage from storage.json
 */
async function load_storage() {
    try {
        if (await fs.pathExists(STORAGE_FILE)) {
            const data = await fs.readJson(STORAGE_FILE);
            return data.storage;
        }
        else {
            // Return default structure
            return {
                knowledge_base: {
                    system_policy_summary: '',
                    greeting_message: ''
                },
                archived_data: {}
            };
        }
    }
    catch (error) {
        console.error('Error loading storage:', error);
        return {
            knowledge_base: {
                system_policy_summary: '',
                greeting_message: ''
            },
            archived_data: {}
        };
    }
}
/**
 * Save storage to storage.json
 */
async function save_storage(storage) {
    try {
        await fs.ensureDir(DATA_DIR);
        const data = { storage };
        await fs.writeJson(STORAGE_FILE, data, { spaces: 2 });
    }
    catch (error) {
        console.error('Error saving storage:', error);
        throw error;
    }
}
/**
 * Get the character count of working memory when serialized
 */
function get_working_memory_size(working_memory) {
    try {
        return JSON.stringify(working_memory).length;
    }
    catch (error) {
        console.error('Error calculating working memory size:', error);
        return 0;
    }
}
/**
 * Check if working memory exceeds the maximum size
 */
function is_working_memory_oversized(working_memory, max_size) {
    return get_working_memory_size(working_memory) > max_size;
}
/**
 * Trim working memory to fit within size constraints
 * This is a simple implementation that removes older action results
 */
function trim_working_memory(working_memory, max_size) {
    const trimmed = JSON.parse(JSON.stringify(working_memory)); // Deep clone
    // If still too large, try removing older action results
    if (is_working_memory_oversized(trimmed, max_size) &&
        typeof trimmed.action_result === 'object' &&
        trimmed.action_result !== null) {
        const action_results = trimmed.action_result;
        const keys = Object.keys(action_results).sort();
        // Remove oldest action results until size is acceptable
        while (keys.length > 0 && is_working_memory_oversized(trimmed, max_size)) {
            const oldest_key = keys.shift();
            if (oldest_key) {
                delete action_results[oldest_key];
            }
        }
    }
    return trimmed;
}
