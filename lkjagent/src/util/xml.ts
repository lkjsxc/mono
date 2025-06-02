/**
 * XML processing utilities for simple XML format (json-like)
 * Only supports simple XML that can be directly converted to JSON
 */

import { tool_action, tool_kind } from '../types/common';

/**
 * Parse simple XML actions from LLM response
 */
export function parse_actions_from_xml(xml: string): tool_action[] {
  const actions: tool_action[] = [];
  
  try {
    // Find all action blocks
    const actionMatches = xml.match(/<action>([\s\S]*?)<\/action>/g);
    
    if (!actionMatches) {
      return actions;
    }
    
    for (const actionXml of actionMatches) {
      const action = parse_single_action(actionXml);
      if (action) {
        actions.push(action);
      }
    }
  } catch (error) {
    console.error('Error parsing XML:', error);
    throw new Error(`XML parsing failed: ${error instanceof Error ? error.message : String(error)}`);
  }
  
  return actions;
}

/**
 * Parse a single action from XML
 */
function parse_single_action(actionXml: string): tool_action | null {
  try {
    const kind = extract_tag_content(actionXml, 'kind');
    const target_path = extract_tag_content(actionXml, 'target_path');
    const source_path = extract_tag_content(actionXml, 'source_path');
    const content = extract_tag_content(actionXml, 'content');
    
    if (!kind || !is_valid_tool_kind(kind)) {
      console.warn('Invalid or missing action kind:', kind);
      return null;
    }
    
    if (!target_path) {
      console.warn('Missing target_path for action:', kind);
      return null;
    }
    
    const action: tool_action = {
      kind: kind as tool_kind,
      target_path,
    };
    
    if (source_path) {
      action.source_path = source_path;
    }
    
    if (content !== null) {
      // Try to parse content as JSON if it looks like JSON
      if (typeof content === 'string' && content.trim().startsWith('{') || content.trim().startsWith('[')) {
        try {
          action.content = JSON.parse(content);
        } catch {
          action.content = content;
        }
      } else {
        action.content = content;
      }
    }
    
    return action;
  } catch (error) {
    console.error('Error parsing single action:', error);
    return null;
  }
}

/**
 * Extract content from XML tag
 */
function extract_tag_content(xml: string, tagName: string): string | null {
  const regex = new RegExp(`<${tagName}>(.*?)<\/${tagName}>`, 's');
  const match = xml.match(regex);
  return match ? match[1].trim() : null;
}

/**
 * Check if a string is a valid tool kind
 */
function is_valid_tool_kind(kind: string): boolean {
  const valid_kinds: tool_kind[] = ['set', 'get', 'rm', 'mv', 'ls', 'search'];
  return valid_kinds.includes(kind as tool_kind);
}

/**
 * Convert JSON object to simple XML for examples
 */
export function json_to_xml(obj: any, rootTag: string = 'root'): string {
  if (typeof obj !== 'object' || obj === null) {
    return `<${rootTag}>${escape_xml_content(String(obj))}</${rootTag}>`;
  }
  
  let xml = `<${rootTag}>`;
  for (const [key, value] of Object.entries(obj)) {
    if (typeof value === 'object' && value !== null) {
      xml += json_to_xml(value, key);
    } else {
      xml += `<${key}>${escape_xml_content(String(value))}</${key}>`;
    }
  }
  xml += `</${rootTag}>`;
  
  return xml;
}

/**
 * Escape XML content
 */
function escape_xml_content(content: string): string {
  return content
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}
