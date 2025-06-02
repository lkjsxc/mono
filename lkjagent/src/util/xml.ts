import { ToolAction } from '../types/common';

/**
 * Error class for XML-related operations
 */
export class XmlError extends Error {
  constructor(message: string, public readonly context?: string) {
    super(message);
    this.name = 'XmlError';
  }
}

/**
 * Escape XML special characters to prevent XML injection
 * @param text - The text to escape
 * @returns Escaped text safe for XML
 */
function escapeXmlContent(text: string): string {
  if (typeof text !== 'string') {
    return String(text);
  }
  
  return text
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&apos;');
}

/**
 * Validate XML tag name to prevent invalid XML generation
 * @param tagName - The tag name to validate
 * @throws XmlError if tag name is invalid
 */
function validateXmlTagName(tagName: string): void {
  if (!tagName || typeof tagName !== 'string') {
    throw new XmlError(`Invalid tag name: expected non-empty string, got ${typeof tagName}`);
  }
  
  // XML tag name rules: must start with letter or underscore, contain only alphanumeric, hyphen, period, underscore
  const validTagNameRegex = /^[a-zA-Z_][a-zA-Z0-9._-]*$/;
  if (!validTagNameRegex.test(tagName)) {
    throw new XmlError(`Invalid XML tag name: '${tagName}'. Tag names must start with a letter or underscore and contain only alphanumeric characters, hyphens, periods, and underscores.`);
  }
}

/**
 * Convert JSON data to XML format for LLM communication with comprehensive error handling
 * @param obj - The object to convert to XML
 * @param rootTag - The root XML tag name
 * @param indent - The current indentation level (for internal use)
 * @returns XML string representation
 * @throws XmlError if conversion fails
 */
export function jsonToXml(obj: any, rootTag: string, indent = ''): string {
  try {
    // Validate root tag
    validateXmlTagName(rootTag);
    
    // Handle null/undefined
    if (obj === null || obj === undefined) {
      return `${indent}<${rootTag}></${rootTag}>`;
    }
    
    // Handle primitive types
    if (typeof obj !== 'object') {
      const escapedContent = escapeXmlContent(String(obj));
      return `${indent}<${rootTag}>${escapedContent}</${rootTag}>`;
    }

    // Prevent circular references
    const seen = new WeakSet();
    function checkCircular(value: any, path: string): void {
      if (value && typeof value === 'object') {
        if (seen.has(value)) {
          throw new XmlError(`Circular reference detected at path: ${path}`);
        }
        seen.add(value);
      }
    }

    checkCircular(obj, rootTag);
    const xmlParts: string[] = [`${indent}<${rootTag}>`];

    try {
      for (const [key, value] of Object.entries(obj)) {
        // Validate each key as XML tag name
        validateXmlTagName(key);
        
        if (Array.isArray(value)) {
          // Handle arrays by creating multiple elements with the same tag
          for (let i = 0; i < value.length; i++) {
            try {
              const itemXml = jsonToXml(value[i], key, indent + '  ');
              xmlParts.push(itemXml);
            } catch (error) {
              throw new XmlError(
                `Failed to convert array item ${i} for key '${key}': ${error instanceof Error ? error.message : String(error)}`,
                `${rootTag}.${key}[${i}]`
              );
            }
          }
        } else if (typeof value === 'object' && value !== null) {
          try {
            const nestedXml = jsonToXml(value, key, indent + '  ');
            xmlParts.push(nestedXml);
          } catch (error) {
            throw new XmlError(
              `Failed to convert nested object for key '${key}': ${error instanceof Error ? error.message : String(error)}`,
              `${rootTag}.${key}`
            );
          }
        } else {
          // Handle primitive values
          const escapedContent = escapeXmlContent(String(value));
          xmlParts.push(`${indent}  <${key}>${escapedContent}</${key}>`);
        }
      }
    } catch (error) {
      if (error instanceof XmlError) {
        throw error;
      }
      throw new XmlError(`Failed to process object properties: ${error instanceof Error ? error.message : String(error)}`, rootTag);
    }

    xmlParts.push(`${indent}</${rootTag}>`);
    return xmlParts.join('\n');
    
  } catch (error) {
    if (error instanceof XmlError) {
      throw error;
    }
    throw new XmlError(`JSON to XML conversion failed: ${error instanceof Error ? error.message : String(error)}`, rootTag);
  }
}

/**
 * Unescape XML entities back to their original characters
 * @param text - The text to unescape
 * @returns Unescaped text
 */
function unescapeXmlContent(text: string): string {
  if (typeof text !== 'string') {
    return String(text);
  }
  
  return text
    .replace(/&quot;/g, '"')
    .replace(/&apos;/g, "'")
    .replace(/&lt;/g, '<')
    .replace(/&gt;/g, '>')
    .replace(/&amp;/g, '&'); // This must be last to avoid double-unescaping
}

/**
 * Extract and validate a single XML tag value
 * @param xml - The XML string to extract from
 * @param tagName - The tag name to extract
 * @param isRequired - Whether this tag is required
 * @param context - Context for error reporting
 * @returns The extracted value or undefined if not required and not found
 * @throws XmlError if required tag is missing or malformed
 */
function extractXmlTag(xml: string, tagName: string, isRequired: boolean = false, context: string = ''): string | undefined {
  try {
    const regex = new RegExp(`<${tagName}>(.*?)<\\/${tagName}>`, 's');
    const match = xml.match(regex);
    
    if (!match) {
      if (isRequired) {
        throw new XmlError(`Required tag '${tagName}' not found`, context);
      }
      return undefined;
    }
    
    const content = match[1];
    
    // Validate that the content doesn't contain unmatched tags
    const openTags = (content.match(/<[^\/][^>]*>/g) || []).length;
    const closeTags = (content.match(/<\/[^>]+>/g) || []).length;
    
    if (openTags !== closeTags) {
      throw new XmlError(`Malformed XML content in tag '${tagName}': unmatched tags detected`, context);
    }
    
    return unescapeXmlContent(content.trim());
    
  } catch (error) {
    if (error instanceof XmlError) {
      throw error;
    }
    throw new XmlError(`Failed to extract tag '${tagName}': ${error instanceof Error ? error.message : String(error)}`, context);
  }
}

/**
 * Parse nested XML content into a JavaScript object
 * @param content - The XML content to parse
 * @param context - Context for error reporting
 * @returns Parsed object or the original content if not XML
 */
function parseNestedXmlContent(content: string, context: string = ''): any {
  if (!content || typeof content !== 'string') {
    return content;
  }
  
  // Check if content contains XML tags
  if (!content.includes('<') || !content.includes('>')) {
    return content;
  }
  
  try {
    const contentObj: any = {};
    const xmlTags = content.match(/<(\w+)>(.*?)<\/\1>/gs);
    
    if (!xmlTags || xmlTags.length === 0) {
      return content; // Return as-is if no valid tags found
    }
    
    for (const tag of xmlTags) {
      const tagMatch = tag.match(/<(\w+)>(.*?)<\/\1>/s);
      if (tagMatch) {
        const [, name, value] = tagMatch;
        if (name && value !== undefined) {
          // Recursively parse nested content
          const parsedValue = parseNestedXmlContent(value, `${context}.${name}`);
          contentObj[name] = parsedValue;
        }
      }
    }
    
    // Return object if we parsed any tags, otherwise return original content
    return Object.keys(contentObj).length > 0 ? contentObj : content;
    
  } catch (error) {
    console.warn(`Failed to parse nested XML content at ${context}: ${error instanceof Error ? error.message : String(error)}`);
    return content; // Return original content if parsing fails
  }
}

/**
 * Validate that an action kind is supported
 * @param kind - The action kind to validate
 * @returns True if valid, false otherwise
 */
function isValidActionKind(kind: string): kind is ToolAction['kind'] {
  const validKinds: ToolAction['kind'][] = [
    'memory_set', 'memory_remove', 'memory_mv',
    'storage_get', 'storage_set', 'storage_search', 'storage_remove', 'storage_ls'
  ];
  return validKinds.includes(kind as ToolAction['kind']);
}

/**
 * Parse XML string to extract actions with comprehensive error handling
 * @param xml - The XML string to parse
 * @returns Array of parsed ToolAction objects
 * @throws XmlError for critical parsing failures (non-action related)
 */
export function parseActionsFromXml(xml: string): ToolAction[] {
  if (!xml || typeof xml !== 'string') {
    throw new XmlError('Invalid input: XML string is required');
  }
  
  const actions: ToolAction[] = [];
  const errors: string[] = [];
  
  try {
    // Check if XML contains actions wrapper
    if (!xml.includes('<action>') || !xml.includes('</action>')) {
      console.warn('No action tags found in XML string');
      return actions;
    }
    
    // Extract all action blocks
    const actionMatches = xml.match(/<action>.*?<\/action>/gs);
    
    if (!actionMatches || actionMatches.length === 0) {
      console.warn('No valid action blocks found in XML');
      return actions;
    }
    
    for (let i = 0; i < actionMatches.length; i++) {
      const actionXml = actionMatches[i];
      const actionContext = `action[${i}]`;
      
      try {
        // Extract required and optional fields
        const kind = extractXmlTag(actionXml, 'kind', true, actionContext);
        const target_path = extractXmlTag(actionXml, 'target_path', false, actionContext);
        const source_path = extractXmlTag(actionXml, 'source_path', false, actionContext);
        const content = extractXmlTag(actionXml, 'content', false, actionContext);
        
        // Validate action kind
        if (!isValidActionKind(kind!)) {
          errors.push(`${actionContext}: Unknown action kind '${kind}'`);
          continue;
        }
        
        // Process content (may contain nested XML)
        let processedContent: any = content;
        if (content) {
          try {
            processedContent = parseNestedXmlContent(content, `${actionContext}.content`);
          } catch (error) {
            console.warn(`Failed to parse content for ${actionContext}: ${error instanceof Error ? error.message : String(error)}`);
            processedContent = content; // Use raw content as fallback
          }
        }
        
        // Create action object
        const action: ToolAction = {
          kind: kind! as ToolAction['kind'],
          target_path: target_path || undefined,
          source_path: source_path || undefined,
          content: processedContent
        };
        
        // Validate action based on kind requirements
        const validationError = validateActionRequirements(action, actionContext);
        if (validationError) {
          errors.push(validationError);
          continue;
        }
        
        actions.push(action);
        
      } catch (error) {
        if (error instanceof XmlError) {
          errors.push(`${actionContext}: ${error.message}`);
        } else {
          errors.push(`${actionContext}: Unexpected error - ${error instanceof Error ? error.message : String(error)}`);
        }
        continue;
      }
    }
    
    // Log all accumulated errors
    if (errors.length > 0) {
      console.warn(`XML parsing completed with ${errors.length} error(s):\n${errors.join('\n')}`);
    }
    
    return actions;
    
  } catch (error) {
    if (error instanceof XmlError) {
      throw error;
    }
    throw new XmlError(`Critical XML parsing failure: ${error instanceof Error ? error.message : String(error)}`);
  }
}

/**
 * Validate that an action has all required fields for its kind
 * @param action - The action to validate
 * @param context - Context for error reporting
 * @returns Error message if validation fails, null if valid
 */
function validateActionRequirements(action: ToolAction, context: string): string | null {
  try {
    switch (action.kind) {
      case 'memory_set':
        if (!action.target_path) return `${context}: memory_set requires target_path`;
        if (action.content === undefined) return `${context}: memory_set requires content`;
        break;
        
      case 'memory_remove':
        if (!action.target_path) return `${context}: memory_remove requires target_path`;
        break;
        
      case 'memory_mv':
        if (!action.source_path) return `${context}: memory_mv requires source_path`;
        if (!action.target_path) return `${context}: memory_mv requires target_path`;
        break;
        
      case 'storage_get':
        if (!action.target_path) return `${context}: storage_get requires target_path`;
        break;
        
      case 'storage_remove':
        if (!action.target_path) return `${context}: storage_remove requires target_path`;
        break;
        
      case 'storage_set':
        if (!action.source_path) return `${context}: storage_set requires source_path`;
        if (!action.target_path) return `${context}: storage_set requires target_path`;
        break;
        
      case 'storage_search':
        if (!action.content) return `${context}: storage_search requires content`;
        break;
        
      case 'storage_ls':
        if (!action.target_path) return `${context}: storage_ls requires target_path`;
        break;
        
      default:
        return `${context}: Unknown action kind '${action.kind}'`;
    }
    
    return null; // No validation errors
    
  } catch (error) {
    return `${context}: Validation error - ${error instanceof Error ? error.message : String(error)}`;
  }
}
