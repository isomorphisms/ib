# Decode source JSON without turning a branched conversation into a linear chat.
def pointer_component: gsub("~"; "~0") | gsub("/"; "~1");
if type != "array" then error("export must be a conversation array") else . end
| to_entries[] as $conversation
| $conversation.value as $value
| if ($value | type) != "object" or ($value.mapping | type) != "object"
  then error("conversation must contain a mapping object") else . end
| {kind:"conversation", source:$source,
   locator:("/" + ($conversation.key|tostring)),
   source_order:$conversation.key,
   original_id:($value.id // $value.conversation_id // null),
   value:($value|del(.mapping))},
  ($value.mapping | to_entries[]
   | if (.value | type) != "object" then error("mapping node must be an object") else . end
   | {kind:"message-node", source:$source,
      locator:("/" + ($conversation.key|tostring) + "/mapping/" + (.key|pointer_component)),
      conversation_locator:("/" + ($conversation.key|tostring)),
      node_key:.key,
      original_id:(.value.message.id // .value.id // null),
      value:.value})
