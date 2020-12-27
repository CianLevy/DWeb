pragma solidity >=0.4.22 <0.6.0;

contract publishobject {
  string node;
  string name;
  string metadata;
  string contenthash;

function SetObjectInfo(string _name, string _metadata, string _contenthash, string _node) public {
name = _name;
metadata = _metadata;
contenthash = _contenthash;
node = _node;
}

function GetObjectInfo() public view returns (string, string, string, string) public { 
return(name, metadata, contenthas, node)
}
}
