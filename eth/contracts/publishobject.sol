pragma solidity >=0.4.22 <0.6.0;

contract publishobject {
 mapping (uint256=> MyObject) public allobjects;
 uint256 public ObjectCount=0;
 struct MyObject { 
  string name;
  string metadata;
  string contenthash;
  string node;
}
function SetObjectInfo(string memory _name, string memory _metadata, string memory _contenthash, string memory _node) public {
    
    ObjectCount += 1;
    allobjects[ObjectCount] = MyObject(_name, _metadata, _contenthash, _node);
}

function GetObjectCounts()public view returns(uint256){
return ObjectCount;
}


function GetObjectInfo(uint256 givenindex)public view returns(string memory, string memory, string memory, string memory){
return(allobjects[givenindex].name,
allobjects[givenindex].metadata,
allobjects[givenindex].contenthash,
allobjects[givenindex].node);
}
}