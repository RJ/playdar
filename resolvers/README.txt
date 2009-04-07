Do not name your resolver with a '.' character in it. This is unsupported and 
subtle stuff will break. Explicitly because we use a configuration getter 
function that uses '.' to separate keys.
