function Q = R2Q(R)

Q=[R(1,1)+R(2,2)+R(3,3)-1, R(3,2)-R(2,3),R(1,3)-R(3,1),R(2,1)-R(1,2)]/2;
Q(1)=sqrt((Q(1)+1)/2);
Q(2:4)=(Q(2:4)./repmat(Q(1),[1,3]))/2;