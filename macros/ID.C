void ID(){
  if(gFile->IsOpen()){
    //fetch(gFile->GetName());
    fetch(const_cast<char*>(gFile->GetName()));
  }
}
