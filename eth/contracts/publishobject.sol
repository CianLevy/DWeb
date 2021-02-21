pragma solidity >=0.4.22 <0.6.0;

contract publishobject {
  mapping (uint256 => ObjectData) public objects;
  uint256 public ObjectCount = 0;

  struct ObjectData { 
    string oid;
    string metadata;
  }

  function SetObjectInfo(string memory oid, string memory metadata) public {    
    objects[ObjectCount] = ObjectData(oid, metadata);
    ObjectCount += 1;
  }

  function GetObjectCount() public view returns(uint256){
    return ObjectCount;
  }


  function GetObjectInfo(uint256 index) public view returns(string memory, string memory){
    return(objects[index].oid, objects[index].metadata);
  }
}
